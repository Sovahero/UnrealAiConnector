#include "LLMCommandComponent.h"

#include "LLMConnectorSubsystem.h"



//----------------------------------------------------------------------
ULLMCommandComponent::ULLMCommandComponent()
{
  PrimaryComponentTick.bCanEverTick = false;
  CommandHandler = nullptr;
}

//----------------------------------------------------------------------
const FLLMCommandStruct& ULLMCommandComponent::GetCommandStruct() const
{
  return CommandStruct;
}

//----------------------------------------------------------------------
void ULLMCommandComponent::RegisterWithLLM()
{
  // Don't register again if already registered
  if(CommandHandler != nullptr)
  {
    return;
  }

  // Create and register the command handler
  CommandHandler = CreateCommandHandler();
  if(CommandHandler != nullptr)
  {
    ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(GetOwner());
    if(LLMConnector != nullptr)
    {
      const FLLMCommandStruct& Struct = GetCommandStruct();
      CommandHandler->InitWithParams(Struct);
      LLMConnector->RegisterCommandHandler(CommandHandler);
      UE_LOG(LLM, Log, TEXT("Registered command: %s for target: %s"), *Struct.Name, *Struct.Target);
    }
  }
}

//----------------------------------------------------------------------
void ULLMCommandComponent::UnregisterFromLLM()
{
  // Clean up handler
  CommandHandler = nullptr;
}

//----------------------------------------------------------------------
void ULLMCommandComponent::BeginPlay()
{
  Super::BeginPlay();

  // Auto-register if enabled
  if(bAutoRegister)
  {
    RegisterWithLLM();
  }
}

//----------------------------------------------------------------------
void ULLMCommandComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  UnregisterFromLLM();
  Super::EndPlay(EndPlayReason);
}

//----------------------------------------------------------------------
ULLMCommandHandlerBase* ULLMCommandComponent::CreateCommandHandler()
{
  ULLMComponentCommandHandler* Handler = NewObject<ULLMComponentCommandHandler>(GetOwner());
  if(Handler)
  {
    Handler->SetOwnerComponent(this);
  }
  return Handler;
}



// Implementation of ULLMComponentCommandHandler
//----------------------------------------------------------------------
void ULLMComponentCommandHandler::SetOwnerComponent(ULLMCommandComponent* InOwnerComponent)
{
  OwnerComponent = InOwnerComponent;
}

//----------------------------------------------------------------------
bool ULLMComponentCommandHandler::CanExecuteCommand(const FLLMResponseBase& ResponseParams)
{
  if(OwnerComponent.IsValid())
  {
    return OwnerComponent->CanExecuteCommand(ResponseParams);
  }
  return false;
}

//----------------------------------------------------------------------
FString ULLMComponentCommandHandler::ExecuteCommand(const FLLMResponseBase& ResponseParams)
{
  if(OwnerComponent.IsValid())
  {
    return OwnerComponent->ExecuteCommand(ResponseParams);
  }
  return TEXT("Error: Command component is no longer valid");
}
