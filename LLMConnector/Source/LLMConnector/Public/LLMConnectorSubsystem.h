
#pragma once

#include "CoreMinimal.h"
#include "LLMCommandHandler.h"
#include "LLMConnectorStructs.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LLMConnectorSubsystem.generated.h"

class ULLMSettings;

DECLARE_LOG_CATEGORY_EXTERN(LLM, Log, All);


DECLARE_MULTICAST_DELEGATE_OneParam(FOnLLMResponseNative, const FLLMResponseBase& /* ResponseParams */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLLMResponse, const FLLMResponseBase&, ResponseParams);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLLMError, ELLMErrorType, ErrorType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProceedCommandsResponse, const FLLMResponseBase&, ResponseParams);



UCLASS()
class LLMCONNECTOR_API ULLMConnectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// Get reference to the LLM Connector subsystem.
	UFUNCTION(BlueprintPure, Category = "LLM|Utilities", meta = (WorldContext = "WorldContextObject"))
	static ULLMConnectorSubsystem* GetLLMConnector(const UObject* WorldContextObject);


	// Function to send message  ✉-->
	UFUNCTION(BlueprintCallable, Category = "LLM|Communication")
	void SendLLMPrompt(const FString& Message, ELLMRole Role);

	// Optionally - to send messages after responding
	UFUNCTION(BlueprintPure, Category = "LLM|Communication")
	bool CanSendLLMPrompt() const;


	// Instructions for JSON response Format	
	UFUNCTION(BlueprintCallable, Category = "LLM|Instructions")
	void SetOverrideInstructionsForResponseFormatTitle(const FString& Title);


	// Prompt History
	UFUNCTION(BlueprintCallable, Category = "LLM|Messages")
	void AddPromptHistory(const FLLMPromptBase& Prompt);
	
	UFUNCTION(BlueprintCallable, Category = "LLM|Messages")
	void RemovePromptHistory(const FLLMPromptBase& Prompt);
	
	UFUNCTION(BlueprintCallable, Category = "LLM|Messages")
	void ClearPromptHistory();

	UFUNCTION(BlueprintPure, Category = "LLM|Messages")
	const TArray<FLLMPromptBase>& GetPromptHistory() const;


	// Set number of reserved messages at the beginning of history
  // These messages won't be removed when history gets trimmed
	UFUNCTION(BlueprintCallable, Category = "LLM|Messages")
	void SetCountReservedMessages(int32 ReservedNum);
	
	UFUNCTION(BlueprintPure, Category = "LLM|Messages")
	int32 GetCountReservedMessages() const;


	// Registering a command handler
	UFUNCTION(BlueprintCallable, Category = "LLM|Commands")
	void RegisterCommandHandler(ULLMCommandHandlerBase* Handler);

	UFUNCTION(BlueprintPure, Category = "LLM|Commands")
	TArray<FLLMCommandStruct> GetParamsRegisterCommands() const;

	UFUNCTION(BlueprintPure, Category = "LLM|Commands")
	FString GetContextCommands(const FString& InfoText = TEXT("Available Commands")) const;

	// Processing command and trying to send it back ✉-->
	void TryProcessCommand(const FLLMResponseBase& ResponseParams);
	
	// Find a handler that can process this command
	UFUNCTION(BlueprintCallable, Category = "LLM|Commands")
	ULLMCommandHandlerBase* FindCommandHandler(const FLLMResponseBase& ResponseParams);

	template <class T>
	T* FindCommandHandlerT(const FLLMResponseBase& ResponseParams)
	{
		for(ULLMCommandHandlerBase* Handler : m_CommandHandlers)
		{
			if(Handler != nullptr && Handler->CanExecuteCommand(ResponseParams))
			{
				if(T* TypedHandler = Cast<T>(Handler))
				{
					return TypedHandler;
				}
			}
		}
		return nullptr;
	}


	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface
	
	/* Response delegates */	
	UPROPERTY(BlueprintAssignable, Category = "LLM|Events")
	FOnLLMResponse OnResponseReceived;
	// Delegates for C++ use (non-UObject handlers)
	FOnLLMResponseNative OnResponseReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "LLM|Events")
	FOnLLMError OnError;
	
	UPROPERTY(BlueprintAssignable, Category = "LLM|Events")
	FOnProceedCommandsResponse OnHandleProceedCommandsResponse;
	
protected:
	static FString ConvertLLMRoleToString(ELLMRole Role);

	// Processing the JSON response from LLM  <--✉
	FLLMResponseBase ProcessLLMResponse(const FString& Response);

	// Finding a suitable handler for the command
	ELLMErrorType TryParseParamsFromResponse(const FString& Response, FLLMResponseBase& OutResponseParams);

	FLLMPromptNode GetInstructionsForResponseFormat() const;
	

	void OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

	
	/* Variables */
	UPROPERTY(Transient)
	const ULLMSettings* m_Settings;
	
	TArray<FLLMPromptBase> m_PromptHistory;

	// To "spread" initial context over messages for a better understanding of llm
	int32 m_ReservedMessages = 0;
	
	UPROPERTY()
	TArray<ULLMCommandHandlerBase*> m_CommandHandlers;
	
	TSet<FHttpRequestPtr> m_ActiveRequests;

	FString m_OverrideInstructionsForResponseFormatTitle;
};
