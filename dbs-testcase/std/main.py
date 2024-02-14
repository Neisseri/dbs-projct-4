from argparse import ArgumentParser
import re

import mysql.connector
from mysql.connector.cursor import MySQLCursor

from judger.testcase import TestPoint

mysql_databases = set(["information_schema", "mysql",
                       "performance_schema", "sys",
                       # Note: PROTECTED is a user-specific database not to avoid dropping
                       "PROTECTED"])
drop_fk_re = re.compile(r"ALTER\s*TABLE\s*(\w+)\s*DROP\s*FOREIGN\s*KEY\s*(\w+);")

fields_re = r"\((`\w+`(?:,\s*`\w+`)*)\)"
pk_regex = re.compile(fr"^\s*PRIMARY\s+KEY.*{fields_re}", flags=re.MULTILINE)
uk_regex = re.compile(fr"^\s*UNIQUE\s+KEY.*{fields_re}", flags=re.MULTILINE)
fk_regex = re.compile(
    fr"^\s*CONSTRAINT.*FOREIGN\s+KEY\s+{fields_re}\s+REFERENCES\s+`(\w*)`\s+{fields_re}", flags=re.MULTILINE)
idx_regex = re.compile(fr"^\s*KEY.*{fields_re}", flags=re.MULTILINE)

rename_regex = re.compile(r"SELECT (.*?) FROM")
def rename_func(m):
    names = m.group(1).split(",")
    for i, name in enumerate(names):
        name = name.strip()
        if name != '*':
            names[i] = f'{name} AS `{name}`'
    return m.group(0).replace(m.group(1), ", ".join(names))


def fields_split(s: str):
    return tuple(s.replace("`", "").replace(" ", "").split(","))


def parse_constraints(cur: MySQLCursor, table):
    cur.execute(f"SHOW CREATE TABLE {table}")
    text = cur.fetchone()[1]
    pk = pk_regex.search(text)
    if pk:
        pk = fields_split(pk.group(1))
    uks = [fields_split(e) for e in uk_regex.findall(text)]
    fks = [{
        "fields": fields_split(e[0]),
        "table": e[1],
        "ref_fields": fields_split(e[2]),
    } for e in fk_regex.findall(text)]
    idxs = [fields_split(e) for e in idx_regex.findall(text)]
    others = {pk, *uks, *(e["fields"] for e in fks)}
    idxs = [idx for idx in idxs if idx not in others]
    # Prepare an empty line
    lines = [""]
    if pk:
        lines.append([f"PRIMARY KEY ({', '.join(pk)});"])
    for fk in fks:
        lines.append(
            [f"FOREIGN KEY ({', '.join(fk['fields'])}) REFERENCES {fk['table']}({', '.join(fk['ref_fields'])});"])
    for uk in uks:
        lines.append([f"UNIQUE ({', '.join(uk)});"])
    for idx in idxs:
        lines.append([f"INDEX ({', '.join(idx)});"])
    return lines


def process_results(cur: MySQLCursor, sql: str, headers, data):

    def process_row(row: tuple):
        line = list(row)
        for i, v in enumerate(line):
            # Float to ".2f" string
            if isinstance(v, float):
                line[i] = f"{v:.2f}"
            if v is None:
                line[i] = "NULL"
        return line

    data = [process_row(row) for row in data]

    flags = TestPoint.generate_flags(sql)
    if sql.startswith("SHOW"):
        if "DATABASE" in sql.upper():
            headers = "DATABASES",
            data = [e for e in data if e[0] not in mysql_databases]
        elif "TABLES" in sql.upper():
            headers = "TABLES",
        else:
            print("Unknown sql", sql)
            exit(-1)
    elif flags.is_desc:
        # Remove other columns
        # reserved column index: 0, 1, 2, 4
        headers = "Field", "Type", "Null", "Default"

        def type_fixer(t: str | bytes):
            # It's weird that mariadb return type as str, but mysql return bytes
            if isinstance(t, bytes):
                t = t.decode()
            if t == "double":
                t = "float"
            return t.upper()
        data = [(row[0], type_fixer(row[1]), row[2], row[4]) for row in data]
        table_name = sql.replace(";", "").split()[1]
        data += parse_constraints(cur, table_name)
    return headers, data

def process_sql(sql: str):
    if "CREATE TABLE" in sql:
        sql = sql.replace("FLOAT", "DOUBLE")
    # sql = rename_regex.sub(rename_func, sql)
    return sql

def finish_sql(cur: MySQLCursor, sql):
    drop_fk = drop_fk_re.match(sql)
    if drop_fk:
        table, key = drop_fk.groups()
        try:
            # Drop the implicit index key
            cur.execute(f"ALTER TABLE {table} DROP KEY {key}")
        except mysql.connector.DatabaseError:
            # User has defined an explicit index key
            pass
    
    row_count_ops = ["INSERT", "UPDATE", "DELETE", "LOAD"]
    if any(sql.startswith(op) for op in row_count_ops):
        print("rows")
        print(cur.rowcount)


# Map for MySQL errno
error_map = {
    1062: "duplicate",  # duplicated entreis for pk or unique
    1068: "primary",    # can not create multiple pk
    1091: "primary",    # can not drop non-existing pk
    1452: "foreign",    # inserted foreign key not existing
    1451: "foreign",    # update/delete fails for foreign key
    1048: "null",       # not null check failed
    1292: "date",       # incorrect date value
}

def run_sql(cur: MySQLCursor, sql: str):
    sql = process_sql(sql)
    try:
        cur.execute(sql)
    except mysql.connector.DatabaseError as e:
        if e.errno in error_map:
            print("!ERROR")
            print(error_map[e.errno])
            return
        raise e
    
    if not cur.with_rows:
        return finish_sql(cur, sql)
    headers, data = process_results(cur, sql, cur.column_names, cur.fetchall())
    print(*headers, sep=',')
    for row in data:
        print(*row, sep=',')
    


def parse_args():
    parser = ArgumentParser()
    parser.add_argument("-u", "--user", required=True)
    parser.add_argument("-p", "--password", required=True)
    parser.add_argument("-P", "--port", default=3306, type=int)
    parser.add_argument("--init", action="store_true")
    parser.add_argument("-b", "--batch", action="store_true")
    parser.add_argument("host")
    return parser.parse_args()


def init(cur: MySQLCursor):
    cur.execute("SHOW DATABASES")
    dbs = [e[0] for e in cur.fetchall()]
    dbs = [db for db in dbs if db not in mysql_databases]
    for db in dbs:
        cur.execute(f"DROP DATABASE {db}")


def main():
    conn = mysql.connector.connect(
        user=args.user,
        password=args.password,
        host=args.host,
        port=args.port,
        buffered=True,
        autocommit=True,
    )
    cur = conn.cursor()
    if args.init:
        init(cur)
    else:
        while True:
            try:
                sql = input().strip()
            except EOFError:
                break
            if sql == "exit":
                break
            if not sql.endswith(";"):
                continue
            run_sql(cur, sql)
            print(f"@{sql}")
    cur.close()
    conn.close()


if __name__ == "__main__":
    args = parse_args()
    main()
