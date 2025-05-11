#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LLMConnectorStructs.h"
#include "LLMAsyncActions.generated.h"



/**
 * Async action to send a message to LLM and wait for response
 */
UCLASS()
class LLMCONNECTOR_API ULLMSendPromptAsyncAction : public UBlueprintAsyncActionBase
{
  GENERATED_BODY()
public:
  /** Delegate called when LLM response is received */
  DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLLMResponseDelegate, const FLLMResponseBase&, ResponseParams);

  /** Delegate called when an error occurs */
  DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLLMErrorDelegate, const FString&, ErrorMessage);

  /** Output delegate for successful response */
  UPROPERTY(BlueprintAssignable)
  FLLMResponseDelegate OnCompleted;

  /** Output delegate for error */
  UPROPERTY(BlueprintAssignable)
  FLLMErrorDelegate OnError;

  /**
   * Send a message to LLM asynchronously
   * 
   * @param WorldContextObject Object with world context (usually self)
   * @param Message Message to send to the LLM
   * @param Role Role of the sender (system, user, assistant)
   * @return Async action object for blueprint node
   */
  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Send LLM Message (Async)"), Category = "LLM|Async")
  static ULLMSendPromptAsyncAction* SendLLMPrompt(UObject* WorldContextObject, const FString& Message, ELLMRole Role = ELLMRole::User);

  // UBlueprintAsyncActionBase interface
  virtual void Activate() override;
  // End of UBlueprintAsyncActionBase interface

private:
  /** The message to send */
  FString Message;

  /** The role of the sender */
  ELLMRole Role;

  /** The world context */
  TWeakObjectPtr<UObject> WorldContextObject;

  /** Callback for LLM response */
  void HandleLLMResponse(const FLLMResponseBase& ResponseParams);

  /** Callback for LLM error */
  void HandleLLMError(const FString& ErrorMessage);
};
