-- @Name: system
-- @Depends:
-- @Flags: 
-- @Description: Check basic database-level operation
-- @Score: 5

-- Make sure no data initialized before
SHOW DATABASES;

-- NOTICE: database "DB" will be used in feature
CREATE DATABASE DB;
CREATE DATABASE DB1;
CREATE DATABASE DB2;

-- Check list all dbs
SHOW DATABASES;
-- If it's not implemented, check directory when in need

-- Check switch db
USE DB;
USE DB2;

-- Check switch db when using some db
CREATE DATABASE DB3;
SHOW DATABASES;

-- Check drop unused db
DROP DATABASE DB1;
SHOW DATABASES;

