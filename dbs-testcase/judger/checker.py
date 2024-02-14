import subprocess
import sys
from pathlib import Path
import os
from time import monotonic_ns

from .testcase import TestCase, TestPoint, Answer
from .error import CheckFailed, PointFailed, TimeLimitExceeded, InitTimeout
from .termcolor import colored as _colored

def colored(*args, attrs=None, force_color=None, **kwargs):
    if attrs is None:
        attrs = ["bold"]
    if force_color is None:
        force_color = True
    kwargs["attrs"] = attrs
    kwargs["force_color"] = force_color
    return _colored(*args, **kwargs)

class TimeAccumulator:
    def __init__(self, timeout_ns):
        self.time_ns = 0
        self.timeout_ns = timeout_ns
        self.start = 0
        self.stop = 0

    def __enter__(self):
        self.start = monotonic_ns()

    def __exit__(self, exc_type, exc_value, exc_tb):
        self.stop = monotonic_ns()
        self.time_ns += self.stop - self.start
        if self.time_ns > self.timeout_ns:
            raise TimeLimitExceeded

    @property
    def past_seconds(self):
        return self.time_ns / 1e9


class Checker:
    def __init__(self, cmd, gen_ans, flags, cases) -> None:
        # judgement meta
        self.gen_ans = gen_ans
        self.prog: subprocess.Popen = None
        self.flags = set(flags or [])
        self.cases = cases
        self.name_to_case = {}
        self.scores = 0
        self.total_scores = 0
        self.time_limiter = TimeAccumulator(3600 * 1e9)
        self.passed_cases = set()
        self.failed_cases = set()
        self.skipped_cases = set()
        self.disabled_cases = set()

        # user program meta
        self.cmd = cmd
        self.env = dict(**os.environ, OMP_NUM_THREADS="1")
        self.cwd = self.env.get("USER_PROG_CWD")
        self.memory_limit = 256 * 1024 * 1024
        # On windows kill() is exactly terminate(), so record the running states manually
        self.runnning = False

    def print_depends(self):
        lines = ["flowchart RL"]
        lines_optional = ["flowchart BT"]
        for name, item in self.name_to_case.items():
            item: TestCase
            content = f"{name}({item.score})"
            if item.flags:
                content += f"\\n({','.join(item.flags)})"
            line = f'{name}["{content}"]'
            if item.optional:
                lines_optional.append(line)
            else:
                lines.append(line)
        for item in self.name_to_case.values():
            if item.name == "optional":
                continue
            for name in item.deps:
                line = f"{item.name} --> {name}"
                if item.optional:
                    lines_optional.append(line)
                else:
                    lines.append(line)
        print("\n    ".join(lines))
        print("\n    ".join(lines_optional))
        exit(0)

    def report(self):
        print("Passed cases:", colored(
            ", ".join(sorted(self.passed_cases)), "green", attrs=["bold"]))
        print("Failed cases:", colored(
            ", ".join(sorted(self.failed_cases)), "red", attrs=["bold"]))
        print("Skipped cases:", colored(
            ", ".join(sorted(self.skipped_cases)), "yellow", attrs=["bold"]))
        print("Disabled cases:", colored(
            ", ".join(sorted(self.disabled_cases)), "grey"))
        if self.scores == self.total_scores:
            color = "green"
        elif self.scores >= self.total_scores * 0.8:
            color = "blue"
        elif self.scores >= self.total_scores * 0.6:
            color = "yellow"
        else:
            color = "red"
        print(
            colored(f"Scores: {self.scores} / {self.total_scores}, Time: {self.time_limiter.past_seconds:.3f}s", color, attrs=['bold']))

    def set_enabled(self, name):
        """
        It is no use now, but may be used in future.
        """
        _case: TestCase = self.name_to_case[name]
        if _case.enabled:
            return
        _case.enabled = True
        if name in self.disabled_cases:
            self.disabled_cases.remove(name)
        for dep in _case.deps:
            self.set_enabled(dep)

    def case_enabled(self, name):
        _case: TestCase = self.name_to_case[name]
        if _case.enabled is not None:
            return _case.enabled
        self.disabled_cases.add(name)
        _case.enabled = False
        for flag in _case.flags:
            if flag not in self.flags:
                return False
        for dep in _case.deps:
            if not self.case_enabled(dep):
                if _case.flags:
                    print(colored(
                        f"[WARNING] You may want to enable testcase {name} but some of its dependancies disabled", "yellow", attrs=["bold"]))
                return False
        _case.enabled = True
        self.disabled_cases.remove(name)
        return True

    def case_optional(self, name):
        _case: TestCase = self.name_to_case[name]
        if _case.optional is not None:
            return _case.optional
        if name == "optional":
            _case.optional = True
            return True
        _case.optional = True
        for dep in _case.deps:
            if self.case_optional(dep):
                return True
        _case.optional = False
        return False

    def read_cases(self, in_dir, ans_dir):
        in_dir = Path(in_dir)
        ans_dir = Path(ans_dir)
        for in_path in sorted(in_dir.iterdir()):
            if in_path.suffix != ".sql":
                print("[WARN] Ignore non-sql file in in_dir:", in_path)
                continue
            ans_path = ans_dir / (in_path.stem + ".ans")
            if not self.gen_ans and not ans_path.exists():
                print("[WARN] sql file has no ans:", in_path)
                continue
            try:
                item = TestCase.from_file(in_path, ans_path, self.gen_ans)
                self.name_to_case[item.name] = item
            except Exception:
                import traceback
                traceback.print_exc()
        if self.cases:
            self.total_scores = sum(self.name_to_case[name].score for name in self.cases)
            print("[INFO] Only run case(s):", ", ".join(self.cases))
            return
        for name in self.name_to_case:
            self.case_optional(name)
            if not self.case_enabled(name):
                self.disabled_cases.add(name)
        self.total_scores = sum(each.score for name, each in self.name_to_case.items()
                                if name not in self.disabled_cases)
        print("[INFO] read", len(self.name_to_case), "cases in total,",
              len(self.name_to_case) - len(self.disabled_cases), "cases enabled")

    def _run_case(self, _case: TestCase):
        if self.gen_ans:
            file = open(_case.ans_path, "w", encoding="utf-8")
        for point in _case.test_points:
            point: TestPoint
            # print("[DEBUG] run sql:" point.sql)
            with self.time_limiter:
                # Restart the killed process and count the starting time
                # Note that it won't cost any time if program is already running
                self.start()
                self.prog.stdin.write(point.sql + "\n")
                self.prog.stdin.flush()
                lines = []
                while True:
                    if self.prog.poll() is not None:
                        raise Exception(f"Progarm exit with code {self.prog.poll()}")
                    line: str = self.prog.stdout.readline()
                    if line.startswith("@"):
                        break
                    # Note that readline will contains the "\n"
                    lines.append(line.strip())
            # print("[DEBUG] read output", lines)
            if self.gen_ans:
                if lines:
                    print(*lines, sep='\n', file=file)
                print(f"@{point.sql}\n", file=file)
                continue
            ans: Answer = point.ans
            out = Answer(lines, ans.flags)
            try:
                ans.check(out)
            except CheckFailed as e:
                print(repr(e))
                raise PointFailed(point.sql)
            except Exception:
                from traceback import print_exc
                print_exc(file=sys.stdout)
                raise PointFailed(point.sql)
        if self.gen_ans:
            file.close()

    def run_case(self, name, skip_dependency_check=False):
        if name in self.passed_cases:
            return True
        _case: TestCase = self.name_to_case[name]
        if not skip_dependency_check:
            if name in self.failed_cases or name in self.skipped_cases or name in self.disabled_cases:
                return False
            for depend_case in _case.deps:
                if depend_case in self.passed_cases:
                    continue
                if depend_case in self.failed_cases or depend_case in self.skipped_cases or not self.run_case(depend_case):
                    self.skipped_cases.add(name)
                    return False
                # run depend_case successfully: continue
        try:
            print("[INFO] Case", name, "is running")
            self._run_case(_case)
            self.scores += _case.score
            self.passed_cases.add(name)
            print(colored(f"[INFO] Case {name} passed", "green"))
            return True
        except KeyboardInterrupt:
            self.kill()
            raise
        except PointFailed as e:
            print(
                colored(f"[ERROR] case {name} failed because {repr(e)}", "red"))
            self.failed_cases.add(name)
            return False
        except TimeLimitExceeded:
            raise
        except Exception:
            from traceback import print_exc
            print_exc(file=sys.stdout)
            print(colored(
                "[CRITICAL] Meet error when running user program, try to restart...", "red", attrs=["bold"]))
            if self.prog.poll() is not None:
                self.runnning = False
            self.failed_cases.add(name)
            return False

    def run(self):
        def count_n_generator(n):
            while True:
                for _ in range(n - 1):
                    yield False
                yield True
        try:
            if self.cases:
                # Skip initialization
                for name in self.cases:
                    self.run_case(name, True)
                self.exit()
            else:
                exit_counter = count_n_generator(5)
                self.init()
                for name in self.name_to_case:
                    if self.run_case(name) and next(exit_counter):
                        self.exit()
                self.exit()
        except TimeLimitExceeded:
            print(colored("[ERROR] Time Limit Exceeded", "red"))
        return self.scores

    def init(self):
        print("[INFO] User program initializing...")
        cmd = self.cmd + ["--init"]
        with self.time_limiter:
            prog = subprocess.Popen(cmd, encoding='utf-8', cwd=self.cwd,
                                    stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                    stderr=sys.stderr, env=self.env)
            try:
                prog.wait(60)
            except subprocess.TimeoutExpired:
                prog.kill()
                raise InitTimeout
        print("[INFO] User program initialized")

    def start(self):
        if self.runnning:
            return

        if self.gen_ans and self.prog and self.prog.returncode:
            print("standard program failed!")
            exit(-1)

        def set_memory_limit():
            import resource
            resource.setrlimit(resource.RLIMIT_AS,
                               (self.memory_limit, self.memory_limit))

        self.prog = subprocess.Popen(self.cmd, encoding='utf-8', env=self.env, stderr=sys.stderr,
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, cwd=self.cwd,
                                     preexec_fn=set_memory_limit if sys.platform == "linux" else None)
        self.runnning = True
        print("[INFO] User program starts")

    def exit(self):
        if not self.runnning:
            return
        state, color, level = "normally", "white", "INFO"
        with self.time_limiter:
            try:
                self.prog.stdin.write("exit\n")
                self.prog.stdin.flush()
                # If exit can not finish in 1minutes, it will kill the process
                self.prog.wait(60)
            except (OSError, BrokenPipeError):
                pass
            except subprocess.TimeoutExpired:
                # Note: If the program gets killed, its states may be undefined
                self.kill()
                state, color, level = "abnormally", "red", "ERROR"
        print(colored(f"[{level}] User program exited {state}", color))
        self.runnning = False

    def kill(self):
        self.prog.kill()
        self.runnning = False
