#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LLMConnectorStructs.h"
#include "LLMCommandHandler.h"

#include "LLMCommandComponent.generated.h"



/**
 * Component that provides LLM command handling capabilities to actors
 * Allows actors to receive and respond to commands from LLM systems
 */
UCLASS(Blueprintable, ClassGroup=(LLMConnector), meta=(BlueprintSpawnableComponent))
class LLMCONNECTOR_API ULLMCommandComponent : public UActorComponent
{
  GENERATED_BODY()

public:
  ULLMCommandComponent();

  // Command configuration properties
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command")
  FLLMCommandStruct CommandStruct;
  
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command")
  bool bAutoRegister = true;

  
  // Command behavior
  UFUNCTION(BlueprintImplementableEvent, Category = "LLM|Command")
  bool CanExecuteCommand(const FLLMResponseBase& ResponseParams);

  UFUNCTION(BlueprintImplementableEvent, Category = "LLM|Command")
  FString ExecuteCommand(const FLLMResponseBase& ResponseParams);

  
  // Get the command parameters structure
  UFUNCTION(BlueprintPure, Category = "LLM|Command")
  const FLLMCommandStruct& GetCommandStruct() const;

  
  // Manually register with LLM system
  UFUNCTION(BlueprintCallable, Category = "LLM|Command")
  void RegisterWithLLM();
  // Unregister from LLM system
  UFUNCTION(BlueprintCallable, Category = "LLM|Command")
  void UnregisterFromLLM();
  

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
  // The actual command handler that communicates with LLMConnectorSubsystem
  UPROPERTY()
  ULLMCommandHandlerBase* CommandHandler;

  // Create a command handler instance
  ULLMCommandHandlerBase* CreateCommandHandler();
};



/**
 * Internal command handler class that connects the component to the LLM system
 * This is implementation detail - users should interact with ULLMCommandComponent
 */
UCLASS()
class LLMCONNECTOR_API ULLMComponentCommandHandler : public ULLMCommandHandlerBase
{
  GENERATED_BODY()

public:
  // Set the owner component
  void SetOwnerComponent(ULLMCommandComponent* InOwnerComponent);

  // ULLMCommandHandlerBase interface
  virtual bool CanExecuteCommand(const FLLMResponseBase& ResponseParams) override;
  virtual FString ExecuteCommand(const FLLMResponseBase& ResponseParams) override;
  // End of ULLMCommandHandlerBase interface

private:
  // Weak pointer to the owning component
  UPROPERTY()
  TWeakObjectPtr<ULLMCommandComponent> OwnerComponent;
};
