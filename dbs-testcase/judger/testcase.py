import re
from operator import itemgetter

from .error import assert_eq, check_constraint_error, CheckFailed


class PointFlags:
    def __init__(self, is_desc, colume_order, order_by, reversed_order) -> None:
        # Note: is_desc means "is description", reversed_order means "ORDER BY ... DESC"
        self.is_desc = is_desc
        self.colume_order = colume_order
        self.order_by = order_by
        self.reversed_order = reversed_order


class Constraint:
    pk_regex = re.compile(
        r"PRIMARY\s+KEY\s+(?P<name>\w*)\((?P<fields>\w+(?:,\s*\w+)*)\);")
    fk_regex = re.compile(
        r"FOREIGN\s+KEY\s+(?P<name>\w*)\((?P<fields>\w+(?:,\s*\w+)*)\)\s+REFERENCES\s(?P<ref_table>\w+)\((?P<ref_fields>\w+(?:,\s*\w+)*)\);")
    uk_regex = re.compile(
        r"UNIQUE\s+(?P<name>\w*)\((?P<fields>\w+(?:,\s*\w+)*)\);")
    idx_regex = re.compile(
        r"INDEX\s+(?P<name>\w*)\((?P<fields>\w+(?:,\s*\w+)*)\);")

    def __init__(self, lines) -> None:
        self.pk = None
        self.fks = []
        self.uks = []
        self.idx = []
        if lines:
            self.parse_lines(lines)

    def parse_lines(self, lines):
        for line in lines:
            if self.pk_regex.match(line):
                m = self.pk_regex.match(line)
                # Make sure at most one primary key
                if self.pk is not None:
                    raise ValueError("Duplicated primary key")
                self.pk = {
                    "name": m.group("name"),
                    "fields": m.group("fields").replace(" ", "").split(",")
                }
            elif self.fk_regex.match(line):
                m = self.fk_regex.match(line)
                fields = m.group("fields").replace(" ", "").split(",")
                ref_fields = m.group("ref_fields").replace(
                    " ", "").split(",")
                if len(fields) != len(ref_fields):
                    raise ValueError(
                        f"{len(fields)} fields mismatches {len(ref_fields)} refer fields")
                self.fks.append({
                    "name": m.group("name"),
                    "table": m.group("ref_table"),
                    "fields": fields,
                    "ref_fields": ref_fields,
                })
            elif self.uk_regex.match(line):
                m = self.uk_regex.match(line)
                self.uks.append({
                    "name": m.group("name"),
                    "fields": m.group("fields").replace(" ", "").split(",")
                })
            elif self.idx_regex.match(line):
                m = self.idx_regex.match(line)
                self.idx.append({
                    "name": m.group("name"),
                    "fields": m.group("fields").replace(" ", "").split(",")
                })
        self.fks.sort(key=lambda e: (e["fields"], e["ref_fields"]))
        self.uks.sort(key=lambda e: e["fields"])
        self.idx.sort(key=lambda e: e["fields"])

    def check(self, other: "Answer.Constraint"):
        # check pk
        assert_eq("Check primary key existence",
                  bool(self.pk), bool(other.pk))
        if self.pk is not None:
            # self.pk["name"] and assert_eq("Check primary key name", self.pk["name"], other.pk["name"])
            assert_eq("Check primary key fields",
                      self.pk["fields"], other.pk["fields"])
        # check fk
        assert_eq("Check foreign keys number",
                  len(self.fks), len(other.fks))
        for fk, ofk in zip(self.fks, other.fks):
            # fk["name"] and assert_eq("Check foreign key name", fk["name"], ofk["name"])
            assert_eq("Check foreign key fields",
                      fk["fields"], ofk["fields"])
            assert_eq("Check foreign key table",
                      fk["table"], ofk["table"])
            assert_eq("Check foreign key refer fields",
                      fk["ref_fields"], ofk["ref_fields"])
        # check uk
        assert_eq("Check union keys number", len(self.uks), len(other.uks))
        for uk, ouk in zip(self.uks, other.uks):
            # uk["name"] and assert_eq("Check union key name", uk["name"], ouk["name"])
            assert_eq("Check union key fields", uk["fields"], ouk["fields"])
        # check idx
        assert_eq("Check index number", len(self.idx), len(other.idx))
        for idx, oidx in zip(self.idx, other.idx):
            # idx["name"] and assert_eq("Checker index name", idx["name"], oidx["name"])
            assert_eq("Check index fields", idx["fields"], oidx["fields"])


class Answer:
    def __init__(self, lines, flags: PointFlags):
        self.flags = flags
        self.headers = lines[0].split(",") if lines else []
        # Ignore order key not in the output headers
        flags.order_by = flags.order_by if flags.order_by and all(
            field in self.headers for field in flags.order_by) else None
        self.constraint = None
        if flags.is_desc:
            if "" in lines:
                blank_line = lines.index("")
                self.constraint = Constraint(lines[blank_line + 1:])
                lines = lines[:blank_line]
            else:
                self.constraint = Constraint([])
        self.data = [line.split(",") for line in lines[1:]]


    def to_regular_data(self):
        if self.flags.colume_order:
            return sorted(",".join(each) for each in self.data)
        # For SELECT *, there can not be duplicated headers
        header_to_idx = {h: idx for idx, h in enumerate(self.headers)}
        indexes = [header_to_idx[header] for header, _ in sorted(self.headers_map.items(), key=itemgetter(1))]
        return sorted(",".join((each[i] for i in indexes)) for each in self.data)

    def match_ordered_headers(self, headers):
        for i, (h1, h2) in enumerate(zip(self.headers, headers)):
            if "." not in h1:
                h2 = h2.split(".", 1)[-1]
            assert_eq(f"Check header {i + 1}", h1, h2)

    def match_unordered_headers(self, headers):
        h1s = set(self.headers)
        h2s = set(headers)
        assert_eq("Check different column names count", len(h1s), len(h2s))
        headers_map = {}
        common = h1s & h2s
        for h in common:
            headers_map[h] = h
        h1s -= common
        h2s -= common
        for h1 in h1s:
            if "." in h1:
                raise CheckFailed(f"Expected header {h1} not found")
        for h2 in h2s:
            h1 = h2.split(".", 1)[-1]
            if h1 in h1s:
                h1s.remove(h1)
                headers_map[h2] = h1
        if h1s:
            raise CheckFailed(f"Expected headers {', '.join(list(h1s))} not found")
        return headers_map


    def check(self, other: "Answer"):
        if self.flags.is_desc:
            self.constraint.check(other.constraint)
        assert_eq("Check columns count", len(self.headers), len(other.headers))
        if not self.headers:
            return
        if self.flags.colume_order:
            self.match_ordered_headers(other.headers)
            other.headers = self.headers
        else:
            self.headers_map = {h: h for h in self.headers}
            other.headers_map = self.match_unordered_headers(other.headers)
        assert_eq("Check rows count", len(self.data), len(other.data))
        # Check !ERROR
        if self.headers[0] == "!ERROR":
            assert_eq("Check error type", check_constraint_error(self.data[0][0]), 
                      check_constraint_error(other.data[0][0]))
            return
        for i, (r1, r2) in enumerate(zip(self.to_regular_data(), other.to_regular_data())):
            assert_eq(f"Check row {i+1}", r1, r2)
        if self.flags.order_by:
            def find_colume_index(headers, field):
                for i, header in enumerate(headers):
                    if "." in field and field == header:
                        return i
                    elif "." not in field and field == header.split(".")[-1]:
                        return i
                else:
                    assert_eq(f"Missing order by field '{field}'", True, False)
            keys = [tuple(float(each[find_colume_index(other.headers, field)]) for field in self.flags.order_by)
                    for each in other.data]
            # Note: Ordered field shouldn't be string, or it will be difficult to process
            if self.flags.reversed_order:
                for i in range(1, len(keys)):
                    assert keys[i - 1] >= keys[i]
            else:
                for i in range(1, len(keys)):
                    assert keys[i - 1] <= keys[i]


class TestPoint:
    order_regex = re.compile(
        r"ORDER\s+BY\s+([^, ]*(?:\s*,\s*[^, ]*)*)\s+(ASC|DESC)?")
    selector_regex = re.compile(r"SELECT(.*?)FROM")
    desc_regex = re.compile(r"DESC\s")

    def __init__(self, sql, ans=None) -> None:
        self.sql: str = sql
        self.flags: PointFlags = self.generate_flags(sql)
        self.ans: Answer = ans

    @staticmethod
    def generate_flags(sql):
        m_order = TestPoint.order_regex.search(sql)
        m_selector = TestPoint.selector_regex.match(sql)
        return PointFlags(
            is_desc=bool(TestPoint.desc_regex.match(sql)),
            colume_order=not(m_selector and "*" in m_selector.group(1)),
            order_by=m_order and m_order.group(1).replace(" ", "").split(","),
            reversed_order=m_order and m_order.group(2) == "DESC",
        )


class TestCase:
    def __init__(self, name, deps, flags, score, desc, sqls, ans_path) -> None:
        self.sqls = sqls
        self.deps = deps
        self.flags = flags
        self.score = score
        self.name = name
        self.desc = desc
        self.ans_path = ans_path
        self.enabled = None
        self.optional = None
        self.test_points = [TestPoint(sql.strip()) for sql in sqls]
    
    @staticmethod
    def from_file(in_path, ans_path, generate_answer):
        with open(in_path) as file:
            text = file.read()
        item = TestCase(
            name=re.search(r"-- @Name: *(.*)", text, re.MULTILINE).group(1),
            deps=re.search(r"-- @Depends: *(.*)", text, re.MULTILINE).group(1).split(),
            flags=re.search(r"-- @Flags: *(.*)", text, re.MULTILINE).group(1).split(),
            desc=re.search(r"-- @Description: *(.*)", text, re.MULTILINE).group(1),
            score=int(re.search(r"-- @Score: *(.*)", text, re.MULTILINE).group(1)),
            sqls=re.findall(r"^[^-].*;\s*$", text, re.MULTILINE),
            ans_path=ans_path,
        )
        if not generate_answer:
            with open(ans_path) as file:
                text = file.read()
                item.load_ans(text)
        return item

    def load_ans(self, text: str) -> bool:
        parts = re.split(r'^@.*$', text, flags=re.MULTILINE)
        # Clear trailing lines after last @line
        parts.pop()
        if len(parts) != len(self.sqls):
            raise ValueError(
                f"ans_file has {len(parts)} parts ended by @line but in_file has {len(self.sqls)} sqls")
        for part, point in zip(parts, self.test_points):
            lines = [line.strip() for line in part.strip().splitlines()]
            point.ans = Answer(lines, point.flags)
