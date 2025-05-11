#include "LLMHelperFunctionLibrary.h"



//----------------------------------------------------------------------
TArray<FString> ULLMHelperFunctionLibrary::SplitString(const FString& SourceString, const FString& Delimiter, bool bCullEmpty)
{
  TArray<FString> Result;
  SourceString.ParseIntoArray(Result, *Delimiter, bCullEmpty);
  return Result;
}

//----------------------------------------------------------------------
bool ULLMHelperFunctionLibrary::ExtractStringAndInt(const TArray<FString>& Parts, FString& OutString, int32& OutInt)
{
  // Initialize with default values
  OutString = FString();
  OutInt = 0;

  // Check if we have enough parts
  if(Parts.Num() < 2)
  {
    return false;
  }

  // Get the string part
  OutString = Parts[0];

  // Get the int part
  if(Parts[1].IsNumeric())
  {
    OutInt = FCString::Atoi(*Parts[1]);
  }
  else
  {
    // Try to extract number from non-numeric string (like "5seconds")
    FString NumericPart;
    for(TCHAR Character : Parts[1])
    {
      if(FChar::IsDigit(Character))
      {
        NumericPart.AppendChar(Character);
      }
      else if(!NumericPart.IsEmpty())
      {
        // Stop at first non-digit after finding digits
        break;
      }
    }

    if(!NumericPart.IsEmpty())
    {
      OutInt = FCString::Atoi(*NumericPart);
    }
    else
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------
bool ULLMHelperFunctionLibrary::ExtractStringAndFloat(const TArray<FString>& Parts, FString& OutString, float& OutFloat)
{
  // Initialize with default values
  OutString = FString();
  OutFloat = 0.0f;

  // Check if we have enough parts
  if(Parts.Num() < 2)
  {
    return false;
  }

  // Get the string part
  OutString = Parts[0];

  // Get the float part
  OutFloat = FCString::Atof(*Parts[1]);

  return true;
}

//----------------------------------------------------------------------
bool ULLMHelperFunctionLibrary::ExtractStringAndString(const TArray<FString>& Parts, FString& OutFirstString, FString& OutSecondString)
{
  // Initialize with default values
  OutFirstString = FString();
  OutSecondString = FString();

  // Check if we have enough parts
  if(Parts.Num() < 2)
  {
    return false;
  }

  // Get the strings
  OutFirstString = Parts[0];
  OutSecondString = Parts[1];

  return true;
}
