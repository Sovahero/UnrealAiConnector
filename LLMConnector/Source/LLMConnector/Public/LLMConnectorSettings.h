
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LLMConnectorSettings.generated.h"



USTRUCT(BlueprintType)
struct FLLMGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY()
	bool bUseTemperature = true;
	/**
   * Controls randomness of generated text
   * 0 is deterministic (always same response), higher values increase randomness
   */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0", Delta = "0.1"), meta = (EditCondition = "bUseTemperature"))
	float Temperature = 1.0f;

	UPROPERTY()
	bool bUseFrequencyPenalty = true;
	/**
	 * Penalizes frequently used tokens
	 * Positive values discourage repetition of common phrases
	 * Negative values encourage repetitive language
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "-2.0", ClampMax = "2.0", UIMin = "-2.0", UIMax = "2.0", Delta = "0.1"), meta = (EditCondition = "bUseFrequencyPenalty"))
	float FrequencyPenalty = 0.8f;

	UPROPERTY()
	bool bUsePresencePenalty = true;
	/**
	 * Penalizes tokens that have already appeared in the text
	 * Positive values encourage use of new tokens
	 * Negative values make model more likely to repeat previous content
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "-2.0", ClampMax = "2.0", UIMin = "-2.0", UIMax = "2.0", Delta = "0.1"), meta = (EditCondition = "bUsePresencePenalty"))
	float PresencePenalty = 0.2f;

	UPROPERTY()
	bool bUseRepetitionPenalty = true;
	/**
	 * Controls how strongly to prevent repetition
	 * Higher values reduce likelihood of repeating exact same phrases
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1.0", ClampMax = "2.0", UIMin = "1.0", UIMax = "2.0", Delta = "0.1"), meta = (EditCondition = "bUseRepetitionPenalty"))
	float RepetitionPenalty = 1.3f;

	UPROPERTY()
	bool bUseMinP = true;
	/**
	 * Minimum probability threshold for token selection
	 * Only tokens with at least this probability are considered
	 * Higher values make output more focused but less creative
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.1"), meta = (EditCondition = "bUseMinP"))
	float MinP = 0.0f;

	UPROPERTY()
	bool bUseTopA = true;
	/**
	 * Adaptive attention scaling parameter
	 * Higher values focus more on relevant context
	 * Helps model stay on topic for complex prompts
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.1"), meta = (EditCondition = "bUseTopA"))
	float TopA = 0.0f;

	UPROPERTY()
	bool bUseTopK = true;
	/**
	 * Consider only the top K most likely tokens at each step
	 * Lower values make output more focused and deterministic
	 * Higher values allow more variety but may decrease quality
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "100", Delta = "1"), meta = (EditCondition = "bUseTopK"))
	int32 TopK = 40;

	UPROPERTY()
	bool bUseTopP = true;
	/**
	 * Probability threshold for token selection
	 * Considers only tokens whose cumulative probability exceeds this threshold
	 * Lower values focus on most likely tokens, higher values allow more diversity
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", Delta = "0.1"), meta = (EditCondition = "bUseTopP"))
	float TopP = 1.0f;

	UPROPERTY()
	bool bUseMaxTokens = false;
	/**
	 * Maximum number of tokens to generate in the response
	 * Higher values allow longer responses but consume more API credits
	 * 1 token ≈ 4 characters or ~3/4 of a word in English
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1", ClampMax = "8192", UIMin = "1", UIMax = "8192", Delta = "8"), meta = (EditCondition = "bUseMaxTokens"))
	int32 MaxTokens = 1024;
};



UCLASS(config = Game, defaultconfig)
class ULLMSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	ULLMSettings()
	{
		CategoryName = TEXT("Plugins");
		SectionName = TEXT("LLM Connector");
	}

	/**
	 * URL endpoint for the LLM API service (default is OpenRouter)
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "API")
	FString ApiURL = TEXT("https://openrouter.ai/api/v1/chat/completions");

	/**
	 * API key for authentication with the service
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "API")
	FString ApiKey = TEXT("sk-or-v1-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	/**
	 * Name of the language model to use (format depends on the service)
	 * https://openrouter.ai/google/gemini-2.0-flash-001 is good
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "API")
	FString ModelName = TEXT("google/gemini-2.0-flash-001");

	/**
	 * Maximum number of messages to keep in conversation history
	 * Older messages beyond this limit will be removed 
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "API")
	int32 MaxHistoryMessages = 10;

	/**
	 * Generation parameters (temperature, top_p, etc.)
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation")
	FLLMGenerationSettings GenerationSettings;

	/**
	 * Main instruction for the JSON response format
	 * This guides the LLM on how to structure its responses 
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "JSON", meta = (MultiLine = true))
	FString ResponseFormatInstructionsText = TEXT("Choose appropriate command for the request.");

	/**
	 * Instructions for the 'parameters' field in JSON responses
	 * Defines what kinds of parameters the LLM should provide
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "JSON")
	FString ParametersInstructionsText = TEXT("[\"param\",\"tag\",\"type\", \"name\", ...],");

	/**
	 * Instructions for the 'message' field in JSON responses
	 * Guidance on what kind of message to return to the player
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "JSON")
	FString MessageInstructionsText = TEXT("\"message to show to player\",");

	/**
	 *	(development builds only)
	 * Instructions for the 'reasoning' field
	 * This field provides insight into the LLM's decision process
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "JSON", meta = (MultiLine = true))
	FString ReasoningInstructionsText = TEXT("\"detailed explanation of your thought process and decision making\"");
};
