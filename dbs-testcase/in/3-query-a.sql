-- @Name: query-a
-- @Depends: table
-- @Flags: query
-- @Description: Small data query task with simple insertion
-- @Score: 3

USE DB;
SELECT * FROM TBL;

INSERT INTO TBL VALUES (1, '1', 1.0);
SELECT * FROM TBL;

INSERT INTO TBL VALUES (2, '2', 2.0), (3, '3', 3.0), (4, '4', 4.0), (5, '5', 5.0), (6, '6', 6.0);
SELECT * FROM TBL WHERE TBL.a >= 0;

INSERT INTO TBL VALUES (2, '2', 2.0), (3, '3', 3.0);
SELECT * FROM TBL WHERE TBL.a > 1;


SELECT a, b, c FROM TBL;
SELECT c, b, a FROM TBL;
SELECT b, c FROM TBL;
SELECT * FROM TBL WHERE TBL.a <= 5 AND TBL.a >= 4;
SELECT c FROM TBL WHERE TBL.a < 5 AND TBL.b = '3';
SELECT b FROM TBL WHERE TBL.b <> '3' AND TBL.b <> '4';
SELECT b FROM TBL WHERE TBL.b <> '3' AND TBL.b = '4';
SELECT * FROM TBL WHERE TBL.c > 2.5;
SELECT * FROM TBL WHERE TBL.c < 3.0;
SELECT a FROM TBL WHERE TBL.b > '2';
SELECT a FROM TBL WHERE TBL.b > '2' AND TBL.b < '3';

