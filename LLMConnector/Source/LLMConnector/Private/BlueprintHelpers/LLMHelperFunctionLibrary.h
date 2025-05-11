#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "LLMHelperFunctionLibrary.generated.h"



/**
 * Helper functions for LLM system and string operations
 */
UCLASS()
class LLMCONNECTOR_API ULLMHelperFunctionLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()
public:
  /**
   * Split a string into an array of strings using a delimiter
   * 
   * @param SourceString The string to split
   * @param Delimiter The character sequence to use as the delimiter
   * @param bCullEmpty Whether to remove empty strings from the result
   * @return Array of split strings
   */
  UFUNCTION(BlueprintCallable, Category = "LLM|String", meta = (DisplayName = "Split String"))
  static TArray<FString> SplitString(const FString& SourceString, const FString& Delimiter, bool bCullEmpty = true);

  /**
   * Extract a string and integer from a part of a split string array
   * Example: ["move", "5"] will return "move" and 5
   * 
   * @param Parts Array of string parts
   * @param OutString The extracted string value
   * @param OutInt The extracted integer value
   * @return True if both parts were successfully extracted
   */
  UFUNCTION(BlueprintCallable, Category = "LLM|String", meta = (DisplayName = "Extract String and Int"))
  static bool ExtractStringAndInt(const TArray<FString>& Parts, FString& OutString, int32& OutInt);

  /**
   * Extract a string and float from a part of a split string array
   * Example: ["move", "5.5"] will return "move" and 5.5
   * 
   * @param Parts Array of string parts
   * @param OutString The extracted string value
   * @param OutFloat The extracted float value
   * @return True if both parts were successfully extracted
   */
  UFUNCTION(BlueprintCallable, Category = "LLM|String", meta = (DisplayName = "Extract String and Float"))
  static bool ExtractStringAndFloat(const TArray<FString>& Parts, FString& OutString, float& OutFloat);

  /**
   * Extract two strings from a part of a split string array
   * Example: ["move", "left"] will return "move" and "left"
   * 
   * @param Parts Array of string parts
   * @param OutFirstString The first extracted string
   * @param OutSecondString The second extracted string
   * @return True if both strings were successfully extracted
   */
  UFUNCTION(BlueprintCallable, Category = "LLM|String", meta = (DisplayName = "Extract Two Strings"))
  static bool ExtractStringAndString(const TArray<FString>& Parts, FString& OutFirstString, FString& OutSecondString);

};
