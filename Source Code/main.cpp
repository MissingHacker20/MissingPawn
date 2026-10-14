#include <iostream>

#include "Others/UCI/UCI.h"
#include "Move/AttackTables.h"

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    AttackTables::initAttackTables();

    UCI::run();

    return 0;
}