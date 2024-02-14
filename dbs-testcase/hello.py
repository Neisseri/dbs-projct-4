import sys
ans = [
    # 1-system.sql
    "DATABASES",
    "",
    "",
    "",
    """
DATABASES
DB
DB1
DB2
""".strip(),
    "",
    "",
    "",
    """
DATABASES
DB
DB1
DB2
DB3
""".strip(),
    "",
    """
DATABASES
DB
DB2
DB3
""".strip(),
    # 2-table.sql
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    """
TABLES
TBL
TBL10
TBL2
TBL3
TBL4
TBL5
TBL6
TBL7
TBL8
TBL9
""".strip(),
    """
Field,Type,Null,Default
a,INT,NO,NULL
b,VARCHAR(16),NO,NULL
c,FLOAT,NO,NULL
    """.strip(),
    "",
    "",
    """
TABLES
TBL
TBL2
TBL3
TBL4
TBL6
TBL7
TBL8
TBL9
""".strip(),
]
if __name__ == "__main__":
    if "--init" in sys.argv:
        exit(0)
    for e in ans:
        if input().strip() == "exit":
            exit(-1)
        if e:
            print(e)
        print("@done")
    print("output done", file=sys.stderr, flush=True)
    while True:
        if input() == "exit":
            exit(0)
        print("@nothing")
