#ifndef TESTCASE1_H
#define TESTCASE1_H

TEST_CASE( "stupid/1=2", "Prove that one equals 2" ){
    int one = 1;
    REQUIRE( one == 2 );
}

#endif // TESTCASE1_H

