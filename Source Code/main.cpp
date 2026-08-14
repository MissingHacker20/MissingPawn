#include <iostream>

#include "Others/UCI/UCI.h"
#include "Move/AttackTables.h"

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    AttackTables::initAttackTables();

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--verify")
        {
            if (AttackTables::verifyAttackTables())
            {
                std::cout << "Attack tables verified: OK\n";
                return 0;
            }
            std::cerr << "Attack table verification FAILED\n";
            return 1;
        }
    }

    UCI::run();

    return 0;
}