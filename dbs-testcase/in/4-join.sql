-- @Name: join
-- @Depends: query-a query-b
-- @Flags: join
-- @Description: Simple two tables join
-- @Score: 3

USE DB;

INSERT INTO TBL7 VALUES (1, '4', 3.0), (4, '5', 4.0), (1, '2', 2.0), (5, '1', 1.0), (3, '4', 2.0), (3, '1', 2.0), (5, '3', 2.0), (5, '5', 3.0);
INSERT INTO TBL7 VALUES (3, '5', 4.0), (1, '5', 1.0);
INSERT INTO TBL7 VALUES (5, '1', 1.0), (2, '5', 5.0), (1, '2', 3.0), (1, '3', 1.0), (1, '3', 2.0), (2, '2', 2.0);
INSERT INTO TBL7 VALUES (3, '4', 3.0), (1, '1', 3.0), (2, '2', 1.0), (3, '3', 5.0), (3, '4', 1.0), (5, '4', 5.0), (4, '1', 2.0);
INSERT INTO TBL7 VALUES (1, '1', 2.0), (5, '2', 2.0), (4, '4', 3.0), (4, '1', 3.0);
INSERT INTO TBL7 VALUES (3, '5', 5.0), (3, '5', 4.0), (2, '1', 4.0), (3, '2', 3.0), (4, '3', 2.0), (4, '1', 4.0), (2, '5', 2.0);
INSERT INTO TBL7 VALUES (5, '4', 5.0), (1, '2', 1.0), (3, '3', 2.0);
INSERT INTO TBL7 VALUES (1, '3', 1.0), (4, '2', 3.0), (5, '2', 3.0), (5, '3', 3.0), (1, '4', 3.0), (3, '4', 5.0);
INSERT INTO TBL7 VALUES (2, '2', 1.0), (1, '3', 2.0), (2, '1', 5.0), (4, '4', 3.0), (4, '3', 1.0), (2, '3', 5.0);
INSERT INTO TBL7 VALUES (1, '2', 1.0), (3, '3', 3.0), (5, '5', 5.0);

INSERT INTO TBL8 VALUES (3, '4', 4.0), (5, '3', 1.0);
INSERT INTO TBL8 VALUES (5, '4', 3.0), (3, '5', 4.0), (4, '2', 3.0), (4, '5', 2.0), (5, '5', 3.0), (4, '2', 2.0);
INSERT INTO TBL8 VALUES (3, '5', 4.0), (5, '4', 4.0), (3, '4', 4.0), (2, '3', 1.0), (5, '1', 1.0), (3, '2', 5.0), (5, '4', 5.0), (5, '3', 1.0), (1, '2', 5.0), (3, '2', 1.0);
INSERT INTO TBL8 VALUES (3, '5', 4.0), (4, '4', 4.0), (2, '5', 4.0), (5, '3', 3.0), (4, '1', 1.0), (2, '3', 1.0);
INSERT INTO TBL8 VALUES (2, '3', 4.0), (1, '2', 1.0), (1, '5', 3.0);
INSERT INTO TBL8 VALUES (2, '4', 4.0), (3, '5', 1.0), (4, '3', 2.0), (2, '5', 3.0), (3, '3', 5.0);
INSERT INTO TBL8 VALUES (5, '5', 3.0), (3, '3', 5.0), (4, '5', 1.0);
INSERT INTO TBL8 VALUES (3, '1', 2.0), (4, '1', 2.0), (2, '2', 5.0), (4, '3', 1.0), (2, '4', 4.0), (1, '5', 2.0), (2, '3', 5.0);
INSERT INTO TBL8 VALUES (1, '4', 1.0), (3, '3', 2.0), (2, '5', 1.0), (5, '5', 5.0), (3, '5', 3.0), (2, '4', 4.0), (5, '1', 4.0), (3, '1', 5.0), (5, '1', 1.0), (5, '2', 4.0);
INSERT INTO TBL8 VALUES (4, '3', 5.0), (5, '1', 2.0);

SELECT * FROM TBL7;
SELECT * FROM TBL8;

SELECT TBL7.a, TBL7.b, TBL7.c, TBL8.a, TBL8.b, TBL8.c FROM TBL7, TBL8 WHERE TBL7.a = TBL8.a;
SELECT TBL7.a, TBL7.b, TBL7.c, TBL8.a, TBL8.b, TBL8.c FROM TBL7, TBL8 WHERE TBL7.b = TBL8.b;
SELECT TBL7.a, TBL7.b, TBL7.c, TBL8.a, TBL8.b, TBL8.c FROM TBL7, TBL8 WHERE TBL7.a = TBL8.a AND TBL7.c < 3.1;
SELECT TBL7.a, TBL7.b, TBL7.c, TBL8.a, TBL8.b, TBL8.c FROM TBL7, TBL8 WHERE TBL7.a = TBL8.a AND TBL8.c < 3.1 AND TBL7.b = '3';
SELECT TBL7.a, TBL7.b, TBL7.c, TBL8.a, TBL8.b, TBL8.c FROM TBL7, TBL8 WHERE TBL7.a = TBL8.a AND TBL7.c < 4.1 AND TBL7.b = '3' AND TBL8.b <> '4';

