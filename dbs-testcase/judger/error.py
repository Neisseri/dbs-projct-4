class PointFailed(Exception):
    pass


class CheckFailed(Exception):
    def __init__(self, msg, ans=None, out=None):
        self.msg = msg
        self.ans = ans
        self.out = out

    def __repr__(self) -> str:
        if self.ans and self.out:
            return f"CheckFailed: {self.msg}, but expected is [{self.ans}] and output is [{self.out}]"
        return f"CheckFailed: {self.msg}"

class TimeLimitExceeded(Exception):
    pass


class InitTimeout(Exception):
    pass



def assert_eq(msg, ans, out):
    if ans != out:
        raise CheckFailed(msg, ans, out)

def check_constraint_error(msg: str):
    err = None
    errs = ["primary", "foreign", "unique"]
    for e in errs:
        if e in msg.lower():
            if err:
                raise CheckFailed(f"Both {e} and {err} in [{msg}]")
            else:
                err = e
    return err
