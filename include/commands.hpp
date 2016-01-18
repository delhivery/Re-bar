#include <marge/graph.hpp>

class Commands {
    private:
        /*enum COMMANDS {
            VADD,
            EADD,
            FIND,
            PATH,
        };*/

        string command;
        shared_ptr<BaseGraph> solver;
    public:
        void run_command() {
        }

        Commands(shared_ptr<BaseGraph> _solver) : solver(_solver) {}
};
