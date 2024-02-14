rm -rf myroot/base/*
rm -rf myroot/global/*
cd dbs-testcase
python3 runner.py -f query join pk fk comb order -- ../build/database -b -r ../myroot/
