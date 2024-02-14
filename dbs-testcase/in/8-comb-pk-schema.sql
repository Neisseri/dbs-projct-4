-- @Name: comb-pk-schema
-- @Depends: pk-schema
-- @Flags: pk comb
-- @Description: Schema alter for combined primary keys
-- @Score: 1

USE DB2;

-- Check create compound pk
ALTER TABLE T0 ADD CONSTRAINT PK PRIMARY KEY (ID, ID2);
DESC T0;
INSERT INTO T0 VALUES (1, 4, '5');
SELECT * FROM T0;
ALTER TABLE T0 DROP PRIMARY KEY;
DESC T0;
