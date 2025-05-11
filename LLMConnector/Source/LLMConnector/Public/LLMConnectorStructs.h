
#pragma once

#include "CoreMinimal.h"
#include "LLMConnectorStructs.generated.h"



UENUM(BlueprintType)
enum class ELLMRole : uint8
{
	System									UMETA(DisplayName = "System"),
	User										UMETA(DisplayName = "User"),
	Assistant								UMETA(DisplayName = "Assistant"),
};

UENUM(BlueprintType)
enum class ELLMErrorType : uint8
{
	None                    UMETA(DisplayName = "No Error"),
	
	InvalidAPIKey           UMETA(DisplayName = "Invalid API Key"),
	RequestInProgress				UMETA(DisplayName = "Request Already In Progress"),
	NetworkError            UMETA(DisplayName = "Network Error"),
	InvalidResponse         UMETA(DisplayName = "Invalid Response"),
	ParseError              UMETA(DisplayName = "Parse Error"),
	MissingFields           UMETA(DisplayName = "Missing Required Fields"),
	Truncated               UMETA(DisplayName = "Response Truncated"),
	JsonParseError          UMETA(DisplayName = "JSON Parse Error"),
	
	UnknownError            UMETA(DisplayName = "Unknown Error")
};



/**
 * Helps create internal nesting for storing and sending text
 */
USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FLLMPromptNode
{
	GENERATED_BODY()

	FString ContentText;

	TMap<FString, TSharedPtr<FLLMPromptNode>> Children;

	
	// Functions
	FLLMPromptNode()
	{}

	FLLMPromptNode(const FString& InContent) : ContentText(InContent)
	{}

	FLLMPromptNode& AddChild(const FString& Key, const FString& Content = TEXT(""))
	{
		TSharedPtr<FLLMPromptNode> NewNode = MakeShared<FLLMPromptNode>(Content);
		Children.Add(Key, NewNode);
		return *NewNode;
	}

	FLLMPromptNode& AddChild(const FString& Key, const FLLMPromptNode& Node)
	{
		TSharedPtr<FLLMPromptNode> NewNode = MakeShared<FLLMPromptNode>(Node);
		Children.Add(Key, NewNode);
		return *NewNode;
	}

	FLLMPromptNode& AddChild(const FString& Key, TSharedPtr<FLLMPromptNode> Node)
	{
		Children.Add(Key, Node);
		return *Node;
	}

	FLLMPromptNode& GetOrAddChild(const FString& Key)
	{
		if(TSharedPtr<FLLMPromptNode>* Found = Children.Find(Key))
		{
			return **Found;
		}
		TSharedPtr<FLLMPromptNode> NewNode = MakeShared<FLLMPromptNode>();
		Children.Add(Key, NewNode);
		return *NewNode;
	}

	bool HasChild(const FString& Key) const
	{
		return Children.Contains(Key);
	}

	TSharedPtr<FLLMPromptNode> GetChild(const FString& Key) const
	{
		if(const TSharedPtr<FLLMPromptNode>* Found = Children.Find(Key))
		{
			return *Found;
		}
		return nullptr;
	}

	void Clear()
	{
		ContentText.Empty();
		Children.Empty();
	}

	FString ToString(int32 Indent = 0) const
	{
		FString Result;

		// If node has content but no children - output as key-value pair on one line
		if(!ContentText.IsEmpty() && Children.Num() == 0)
		{
			Result += FString::ChrN(Indent, ' ') + ContentText;
			return Result;
		}

		// If node has content - output it on separate line
		if(!ContentText.IsEmpty())
		{
			Result += FString::ChrN(Indent, ' ') + ContentText + TEXT("\n");
		}

		// Process children
		for(const auto& Child : Children)
		{
			// If child has no children - output as key-value pair on one line
			if(Child.Value->Children.Num() == 0)
			{
				Result += FString::ChrN(Indent + 2, ' ') + Child.Key;
				if(!Child.Value->ContentText.IsEmpty())
				{
					Result += TEXT(": ") + Child.Value->ContentText + TEXT("\n");
				}
				else
				{
					Result += TEXT("\n");
				}
			}
			else
			{
				// If child has children - output hierarchically
				Result += FString::ChrN(Indent + 2, ' ')
					+ Child.Key + TEXT(":\n")
					+ Child.Value->ToString(Indent + 4);
			}
		}

		return Result;
	}
};



/**
 * Storing response fields from an LLM
 */
USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FLLMResponseBase
{
	GENERATED_BODY()
	
	/** Main message content from LLM response */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Response")
	FString Message;
	
	/** Command identifier from the structured response */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Response")
	FString Command;
	
	/** Target system or object for the command */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Response")
	FString Target;
	
	/** Optional parameters for the command */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Response")
	TArray<FString> Parameters;
	
	/** Reasoning or explanation provided by the LLM. Only Dev build */
	FString Reasoning;
	
	
	FString ToString() const
	{
		FString Result;
		Result += TEXT("Message: ") + Message + TEXT("\n");
		Result += TEXT("Target: ") + Target + TEXT("\n");
		Result += TEXT("Command: ") + Command + TEXT("\n");
		if(!Parameters.IsEmpty())
		{
			Result += TEXT("Parameters: ") + FString::Join(Parameters, TEXT(", ")) + TEXT("\n");
		}

#if !UE_BUILD_SHIPPING
		Result += TEXT("Reasoning: ") + Reasoning + TEXT("\n");
#endif

		return Result;
	}

	// use as "by {command} command with {parameters} parameters.."
	FString GetFormatString(const FString& Text) const
	{
		FStringFormatNamedArguments Args;
		Args.Add(TEXT("command"), Command);
		Args.Add(TEXT("target"), Target);
		Args.Add(TEXT("message"), Message);
		Args.Add(TEXT("parameters"), FString::Join(Parameters, TEXT(", ")));
		return FString::Format(*Text, Args);
	}
};



/**
 * Containing user message fields intended for LLM.
 */
USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FLLMPromptBase
{
	GENERATED_BODY()

	/** Role of the message sender (System, User, Assistant) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Prompt")
	ELLMRole Role = ELLMRole::User;

	/** Message content text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Prompt", meta = (MultiLine = true))
	FString Content;
	

	FLLMPromptBase()
	{}

	FLLMPromptBase(const ELLMRole& InRole, const FString& InContent) : Role(InRole), Content(InContent)
	{}

	bool operator ==(const FLLMPromptBase& Other) const
	{
		return Role == Other.Role &&
			Content == Other.Content;
	}
};



/**
 * For commands intended for the LLM.
 */
USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FLLMCommandStruct
{
	GENERATED_BODY()
	
	/** Target system or object to receive the command */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command")
	FString Target;
    
	/** Command name identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command")
	FString Name;

	/** LLM-readable description of what the command does */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command", meta = (MultiLine = true))
	FString Description;

	/** Example uses of the command to guide LLM */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command", meta = (MultiLine = true))
	TArray<FString> Examples;

	/** Optional message to prepend when using this command */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command", meta = (MultiLine = true))
	FString PrefixMessage;

	/** Optional helper class associated with this command */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM|Command", meta = (AllowAbstract = false))
	TSubclassOf<UObject> HelperClass;
};
