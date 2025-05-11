
#pragma once

#include "CoreMinimal.h"
#include "LLMConnectorStructs.h"

#include "LLMCommandHandler.generated.h"



/**
 * Base class for command handlers
 * Implement this interface to create handlers for specific object types
 */
UCLASS(Abstract, Blueprintable)
class LLMCONNECTOR_API ULLMCommandHandlerBase : public UObject
{
  GENERATED_BODY()
public:

  /**
   * For C++ override: Override these methods directly in your C++ subclass
   */

  /**
   * Initialize handler with command parameters
   */
  UFUNCTION(BlueprintCallable, Category = "LLM|Commands", meta = (DisplayName = "InitWithParams"))
  virtual void InitWithParams(const FLLMCommandStruct& Params)
  {
    SetParams(Params);
    InitWithParamsBP(Params);
  }

  /**
   * Executes the command and returns the result
   * 
   * @return String to send back to llm.
   */
  virtual FString ExecuteCommand(const FLLMResponseBase& ResponseParams)
  {
    return ExecuteCommandBP(ResponseParams);// default TEXT("")
  }

  /**
   * Checks if this handler can process the given response
   * 
   * @return Can execute command.
   */
  virtual bool CanExecuteCommand(const FLLMResponseBase& ResponseParams)
  {
    return CanExecuteCommandBP(ResponseParams);// default false
  }


  UFUNCTION(BlueprintCallable, Category = "LLM|Commands")
  void SetParams(const FLLMCommandStruct& ResponseParams)
  {
    m_Params = ResponseParams;
  }

  UFUNCTION(BlueprintPure, Category = "LLM|Commands")
  const FLLMCommandStruct& GetParams()
  {
    return m_Params;
  }

  
protected:

  /**
   * For Blueprint: These events will be called automatically
   * Override these methods directly in your BP subclass
   */

  // Initialize handler with command parameters, called  after main funct
  UFUNCTION(BlueprintImplementableEvent, Category = "LLM|Commands", meta = (DisplayName = "InitParams"))
  void InitWithParamsBP(const FLLMCommandStruct& Params);

  // Executes the command and returns the result
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "LLM|Commands", meta = (DisplayName = "ExecuteCommand"))
  FString ExecuteCommandBP(const FLLMResponseBase& ResponseParams);

  // Checks if this handler can process the given response
  UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "LLM|Commands", meta = (DisplayName = "CanExecuteCommand"))
  bool CanExecuteCommandBP(const FLLMResponseBase& ResponseParams);

  
  FLLMCommandStruct m_Params = FLLMCommandStruct();
};
