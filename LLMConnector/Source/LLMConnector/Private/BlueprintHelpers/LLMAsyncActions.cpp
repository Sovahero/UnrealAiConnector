#include "LLMAsyncActions.h"

#include "LLMConnectorSubsystem.h"



//----------------------------------------------------------------------
ULLMSendPromptAsyncAction* ULLMSendPromptAsyncAction::SendLLMPrompt(UObject* WorldContextObject, const FString& Message, ELLMRole Role)
{
  ULLMSendPromptAsyncAction* Action = NewObject<ULLMSendPromptAsyncAction>();
  Action->Message = Message;
  Action->Role = Role;
  Action->WorldContextObject = WorldContextObject;
  return Action;
}

//----------------------------------------------------------------------
void ULLMSendPromptAsyncAction::Activate()
{
  // Get the LLM connector subsystem
  ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(WorldContextObject.Get());
  if(!LLMConnector)
  {
    OnError.Broadcast("Failed to get LLM Connector subsystem");
    SetReadyToDestroy();
    return;
  }

  // Check if we can send a prompt now
  if(!LLMConnector->CanSendLLMPrompt())
  {
    OnError.Broadcast("Another request is already in progress");
    SetReadyToDestroy();
    return;
  }

  // Set up response delegates
  FDelegateHandle ResponseHandle = LLMConnector->OnResponseReceivedNative.AddUObject(this, &ULLMSendPromptAsyncAction::HandleLLMResponse);
  FScriptDelegate ErrorDelegate;
  ErrorDelegate.BindUFunction(this, "HandleLLMError");
  LLMConnector->OnError.Add(ErrorDelegate);

  // Send the prompt
  LLMConnector->SendLLMPrompt(Message, Role);
}

//----------------------------------------------------------------------
void ULLMSendPromptAsyncAction::HandleLLMResponse(const FLLMResponseBase& ResponseParams)
{
  // Remove delegate bindings
  if(ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(WorldContextObject.Get()))
  {
    LLMConnector->OnResponseReceivedNative.RemoveAll(this);
    LLMConnector->OnError.RemoveAll(this);
  }

  // Broadcast the response
  OnCompleted.Broadcast(ResponseParams);

  // Mark for garbage collection
  SetReadyToDestroy();
}

//----------------------------------------------------------------------
void ULLMSendPromptAsyncAction::HandleLLMError(const FString& ErrorMessage)
{
  // Remove delegate bindings
  if(ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(WorldContextObject.Get()))
  {
    LLMConnector->OnResponseReceivedNative.RemoveAll(this);
    LLMConnector->OnError.RemoveAll(this);
  }

  // Broadcast the error
  OnError.Broadcast(ErrorMessage);

  // Mark for garbage collection
  SetReadyToDestroy();
}
