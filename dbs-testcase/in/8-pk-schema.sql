-- @Name: pk-schema
-- @Depends: pk
-- @Flags: pk
-- @Description: Schema alter for primary keys
-- @Score: 1

USE DB2;

-- Prepare data
CREATE TABLE T0 (ID INT NOT NULL, ID2 INT NOT NULL, NAME VARCHAR(16));
INSERT INTO T0 VALUES (1, 1, '1'), (2, 2, '2'), (3, 3, '3'), (1, 4, '4');
SELECT * FROM T0 WHERE T0.ID > 0 ;

-- Check error: duplication on creating pk
ALTER TABLE T0 ADD CONSTRAINT PK PRIMARY KEY (ID);

-- Check add pk
DELETE FROM T0 WHERE T0.NAME = '4';
ALTER TABLE T0 ADD CONSTRAINT PK PRIMARY KEY (ID);
DESC T0;

-- Check error: one table has only one pk at most
ALTER TABLE T0 ADD CONSTRAINT PK PRIMARY KEY (ID, ID2);
DESC T0;


-- Check drop pk
INSERT INTO T0 VALUES (1, 4, '4');
ALTER TABLE T0 DROP PRIMARY KEY;
INSERT INTO T0 VALUES (1, 4, '4');
SELECT * FROM T0 WHERE T0.ID > 0 ;
DESC T0;

-- Fail to drop
ALTER TABLE T0 DROP PRIMARY KEY;


