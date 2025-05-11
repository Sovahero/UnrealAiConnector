// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LLMContextStructs.generated.h"



USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FGameContextBase
{
	GENERATED_BODY()

	/* Basic description of the world and the role of AI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context", meta = (MultiLine = true))
	FString WorldDescription;

	/* Description of the AI's role in the dialog */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context", meta = (MultiLine = true))
	FString AIRole;
};



USTRUCT(BlueprintType)
struct LLMCONNECTOR_API FContextDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString FullName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ShortName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FText> AlternativeNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Tag;

	FString ToString() const
	{
		FString Result;

		// Main
		Result += FString::Printf(TEXT("  Full Name: %s\n"), *FullName);
		Result += FString::Printf(TEXT("  Description: %s\n"), *Description);

		// Optional
		if(!ShortName.IsEmpty())
		{
			Result += FString::Printf(TEXT("  Short Name: %s\n"), *ShortName);
		}

		if(AlternativeNames.Num() > 0)
		{
			Result += TEXT("  Alternative Names: ");
			static TArray<FString> StrNames; StrNames.Reset();
			Algo::Transform(AlternativeNames, StrNames, [](const FText& Text) { return Text.ToString(); });
			Result += FString::Join(StrNames, TEXT(", "));
			Result += TEXT("\n");
		}

		if(!Tag.IsEmpty())
		{
			Result += FString::Printf(TEXT("  Tag: %s\n"), *Tag);
		}

		Result += TEXT("\n");
		return Result;
	}

	FLLMPromptNode ToPromptNode() const
	{
		FLLMPromptNode Node;

		// Main
		Node.ContentText = Description;

		// Optional
		if(!ShortName.IsEmpty())
		{
			Node.AddChild("Short Name", ShortName);
		}

		if(AlternativeNames.Num() > 0)
		{
			static TArray<FString> StrNames; StrNames.Reset();
			Algo::Transform(AlternativeNames, StrNames, [](const FText& Text) { return Text.ToString(); });
			Node.AddChild("Alternative Names", FString::Join(StrNames, TEXT(", ")));
		}

		if(!Tag.IsEmpty())
		{
			Node.AddChild("Tag", Tag);
		}

		return Node;
	}

	// Templates
	template <typename T>
	static FString ConvertContextMapToString(const TMap<T, FContextDescription>& Map)
	{
		FString Result;
		for(const auto& Pair : Map)
		{
			Result += Pair.Value.ToString();
		}
		return Result;
	}

	template <typename T>
	static void ConvertContextMapToNode(const TMap<T, FContextDescription>& Map, FLLMPromptNode& OutParentNode)
	{
		for(const auto& Pair : Map)
		{
			OutParentNode.AddChild(Pair.Value.FullName, Pair.Value.ToPromptNode());
		}
	}
};
