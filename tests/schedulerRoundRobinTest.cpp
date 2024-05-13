#include "round_robin.hpp"
#include "iostream"
#include <ostream>

#define CHECK_ERR(a, b)                                                        \
    {                                                                          \
        if (a != QDMI_SUCCESS)                                                 \
        {                                                                      \
            std::cout << std::endl << "[Error]: " << a << " at " << b;         \
        }                                                                      \
    }

int main(){

    QInfo info;
    QDMI_Session session = NULL;
    int err = QInfo_create(&info);
    CHECK_ERR(err, "QInfo_create");

    err = QDMI_session_init(info, &session);

    //QuantumTask dummyTask;
    std::vector<QuantumTask> quantumTasks;
    quantumTasks.push_back(QuantumTask());
    quantumTasks.push_back(QuantumTask());
    
    scheduler(&quantumTasks);

    for(auto& quantumTask: quantumTasks){
        bool isAssigned = quantumTask.scheduled_qpu != nullptr;
        if(!isAssigned)
            std::cout << "Something is wrong, I can feel it!" << std::endl;
    }

    
}