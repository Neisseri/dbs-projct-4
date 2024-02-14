-- @Name: unique
-- @Depends: optional
-- @Flags: unique
-- @Description: Unique index constraint
-- @Score: 1

USE DB;

-- Check unique creation
CREATE TABLE T102 (ID INT, UNI INT);
INSERT INTO T102 VALUES (1, 1), (2, 2), (3, 3), (4, 1);
ALTER TABLE T102 ADD UNIQUE UNQ_UNI(UNI);
DESC T102;
DELETE FROM T102 WHERE T102.ID = 4;
ALTER TABLE T102 ADD UNIQUE UNQ_UNI(UNI);
DESC T102;

-- Check unique insertion
INSERT INTO T102 VALUES (4,1);
INSERT INTO T102 VALUES (4,4);
SELECT * FROM T102;

ALTER TABLE T102 DROP INDEX UNQ_UNI;
