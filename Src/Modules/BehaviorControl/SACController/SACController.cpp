#include "SACController.h"

thread_local SACController* SACController::theInstance = nullptr;

SACController::SACController() : 
    tcpConnection("10.0.50.99", 5050, 0, 0)
{
    printf("SACController created\n");
    
    if(tcpConnection.connected())
    {
        printf("Connected to host\n");
    }
    else
    {
        printf("Failed to connect to host\n");
    }
    theInstance = this;
}

SACController::~SACController()
{
    printf("SACController destroyed\n");
    theInstance = nullptr;
}

void SACController::update(DummyRepresentation& dummyRepresentation)
{
    dummyRepresentation.dummy = 1;
}

MAKE_MODULE(SACController, behaviorControl);