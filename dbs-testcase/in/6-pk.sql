-- @Name: pk
-- @Depends: query-a
-- @Flags: pk
-- @Description: Basic primary key testing
-- @Score: 2

USE DB2;

-- Check create table with pk
CREATE TABLE T1 (ID INT, NAME VARCHAR(16), PRIMARY KEY (ID));

-- Check duplication handle
INSERT INTO T1 VALUES (1, '1'), (2, '2'), (3, '3');
INSERT INTO T1 VALUES (4, '1'), (5, '2'), (6, '3');
INSERT INTO T1 VALUES (4, '8');
INSERT INTO T1 VALUES (7, '7'), (9, '9');
SELECT * FROM T1 WHERE T1.ID > 0;
