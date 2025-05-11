#include "LLMConnectorSubsystem.h"

#include "LLMConnectorSettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

DEFINE_LOG_CATEGORY(LLM);



//----------------------------------------------------------------------
ULLMConnectorSubsystem* ULLMConnectorSubsystem::GetLLMConnector(const UObject* WorldContextObject)
{
  if(WorldContextObject != nullptr)
  {
    if(UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
    {
      if(UGameInstance* GameInstance = World->GetGameInstance())
      {
        return GameInstance->GetSubsystem<ULLMConnectorSubsystem>();
      }
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::SendLLMPrompt(const FString& Message, ELLMRole Role)
{
  // Get settings
  if(m_Settings == nullptr || m_Settings->ApiKey.IsEmpty())
  {
    UE_LOG(LLM, Error, TEXT("API Key not set in Project Settings"));
    OnError.Broadcast(ELLMErrorType::InvalidAPIKey);
    return;
  }

  // Check if similar request is already in progress
  if(m_ActiveRequests.Num() > 0)
  {
    UE_LOG(LLM, Warning, TEXT("Another request is already in progress, waiting..."));
    return;
  }

  // Create HTTP request
  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

  // Store request
  m_ActiveRequests.Add(HttpRequest);

  // Setup the request
  HttpRequest->SetURL(m_Settings->ApiURL);
  HttpRequest->SetVerb(TEXT("POST"));
  HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
  HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *m_Settings->ApiKey));

  // Create JSON payload
  TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
  JsonObject->SetStringField(TEXT("model"), m_Settings->ModelName);

  // Create messages array
  TArray<TSharedPtr<FJsonValue>> MessagesArray;

  // Add new user message
  FLLMPromptBase UserMessage(Role, Message);
  AddPromptHistory(UserMessage);

  // Max history messages
  if(m_PromptHistory.Num() > m_Settings->MaxHistoryMessages + m_ReservedMessages)// for context messages
  {
    int32 ToRemove = m_PromptHistory.Num() - m_Settings->MaxHistoryMessages - m_ReservedMessages;
    m_PromptHistory.RemoveAt(m_ReservedMessages, ToRemove);
  }

  // Add instructions to the response format at the end of the messages to avoid hallucinating llm
  if(Role == ELLMRole::User)
  {
    FLLMPromptBase FormatMessage(ELLMRole::System, GetInstructionsForResponseFormat().ToString());
    RemovePromptHistory(FormatMessage);// Delete previous message to save context
    AddPromptHistory(FormatMessage);
  }

  // Add all messages from history
  for(const FLLMPromptBase& HistoryMessage : m_PromptHistory)
  {
    TSharedPtr<FJsonObject> MessageObject = MakeShared<FJsonObject>();
    MessageObject->SetStringField(TEXT("role"), ConvertLLMRoleToString(HistoryMessage.Role));
    MessageObject->SetStringField(TEXT("content"), HistoryMessage.Content);
    MessagesArray.Add(MakeShared<FJsonValueObject>(MessageObject));
  }

  JsonObject->SetArrayField(TEXT("messages"), MessagesArray);

  // Generation settings
  if(m_Settings->GenerationSettings.bUseTemperature)
  {
    JsonObject->SetNumberField(TEXT("temperature"), m_Settings->GenerationSettings.Temperature);
  }
  if(m_Settings->GenerationSettings.bUseFrequencyPenalty)
  {
    JsonObject->SetNumberField(TEXT("frequency_penalty"), m_Settings->GenerationSettings.FrequencyPenalty);
  }
  if(m_Settings->GenerationSettings.bUsePresencePenalty)
  {
    JsonObject->SetNumberField(TEXT("presence_penalty"), m_Settings->GenerationSettings.PresencePenalty);
  }
  if(m_Settings->GenerationSettings.bUseRepetitionPenalty)
  {
    JsonObject->SetNumberField(TEXT("repetition_penalty"), m_Settings->GenerationSettings.RepetitionPenalty);
  }
  if(m_Settings->GenerationSettings.bUseMinP)
  {
    JsonObject->SetNumberField(TEXT("min_p"), m_Settings->GenerationSettings.MinP);
  }
  if(m_Settings->GenerationSettings.bUseTopA)
  {
    JsonObject->SetNumberField(TEXT("top_a"), m_Settings->GenerationSettings.TopA);
  }
  if(m_Settings->GenerationSettings.bUseTopK)
  {
    JsonObject->SetNumberField(TEXT("top_k"), m_Settings->GenerationSettings.TopK);
  }
  if(m_Settings->GenerationSettings.bUseTopP)
  {
    JsonObject->SetNumberField(TEXT("top_p"), m_Settings->GenerationSettings.TopP);
  }
  if(m_Settings->GenerationSettings.bUseMaxTokens)
  {
    JsonObject->SetNumberField(TEXT("max_tokens"), m_Settings->GenerationSettings.MaxTokens);
  }

  // Add response_format as object
  TSharedPtr<FJsonObject> ResponseFormatObject = MakeShared<FJsonObject>();
  ResponseFormatObject->SetStringField(TEXT("type"), TEXT("json_object"));
  JsonObject->SetObjectField(TEXT("response_format"), ResponseFormatObject);

  // stop response as user
  TArray<TSharedPtr<FJsonValue>> StopArray;
  StopArray.Add(MakeShared<FJsonValueString>(TEXT("USER")));
  JsonObject->SetArrayField(TEXT("stop"), StopArray);

  // Convert JSON to string
  FString JsonString;
  TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
  FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

  // Set request content
  HttpRequest->SetContentAsString(JsonString);

  // Bind callback
  HttpRequest->OnProcessRequestComplete().BindUObject(this, &ULLMConnectorSubsystem::OnHttpResponse);

  // Send request
  HttpRequest->ProcessRequest();

  FString LogJsonString = JsonString;
  LogJsonString.ReplaceInline(TEXT("\\n"), TEXT("\n"));
  UE_LOG(LLM, Log, TEXT("Sending request: %s"), *LogJsonString);
}

//----------------------------------------------------------------------
FString ULLMConnectorSubsystem::ConvertLLMRoleToString(ELLMRole Role)
{
  switch(Role)
  {
  case ELLMRole::System:
    return TEXT("system");
  case ELLMRole::User:
    return TEXT("user");
  case ELLMRole::Assistant:
    return TEXT("assistant");
  }
  return TEXT("user");
}

//----------------------------------------------------------------------
FLLMResponseBase ULLMConnectorSubsystem::ProcessLLMResponse(const FString& Response)
{
  // Try to parse a command from the response
  FLLMResponseBase ResponseParams;

  // If parsing failed, set message to original response
  ELLMErrorType ParseResult = TryParseParamsFromResponse(Response, ResponseParams);
  if(ParseResult != ELLMErrorType::None)
  {
    ResponseParams.Message = Response;
  }

  // Successfully parsed command
  return ResponseParams;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::TryProcessCommand(const FLLMResponseBase& ResponseParams)
{
  // Try to send system prompt
  if(ULLMCommandHandlerBase* Command = FindCommandHandler(ResponseParams))
  {
    const FString& StringForLLM = Command->ExecuteCommand(ResponseParams);
    if(!StringForLLM.IsEmpty())
    {
      SendLLMPrompt(StringForLLM, ELLMRole::System);
    }
  }
}

//----------------------------------------------------------------------
bool ULLMConnectorSubsystem::CanSendLLMPrompt() const
{
  return m_ActiveRequests.Num() == 0;
}

//----------------------------------------------------------------------
FLLMPromptNode ULLMConnectorSubsystem::GetInstructionsForResponseFormat() const
{
  FLLMPromptNode PromptNode;
  
  TArray<FString> CommandNames;
  TArray<FString> Targets;
  for(const FLLMCommandStruct& Command : GetParamsRegisterCommands())
  {
    CommandNames.AddUnique(Command.Name);
    Targets.AddUnique(Command.Target);
  }

  if(!m_OverrideInstructionsForResponseFormatTitle.IsEmpty())
  {
    PromptNode.ContentText = m_OverrideInstructionsForResponseFormatTitle;
  }
  else
  {
    PromptNode.ContentText = m_Settings->ResponseFormatInstructionsText;
  }

  FLLMPromptNode JsonNode;
  JsonNode.AddChild("{");
  JsonNode.AddChild(" \"command\"", TEXT("\"") + FString::Join(CommandNames, TEXT("|")) + TEXT("\","));
  JsonNode.AddChild(" \"target\"", TEXT("\"") + FString::Join(Targets, TEXT("|")) + TEXT("\","));
  JsonNode.AddChild(" \"parameters\"", m_Settings->ParametersInstructionsText);
  JsonNode.AddChild(" \"message\"", m_Settings->MessageInstructionsText);
  
#if !UE_BUILD_SHIPPING
  JsonNode.AddChild(" \"reasoning\"", m_Settings->ReasoningInstructionsText);
#endif
  
  JsonNode.AddChild("}");
  
  PromptNode.AddChild("Use this JSON object to give your answer", JsonNode);

  return PromptNode;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::SetOverrideInstructionsForResponseFormatTitle(const FString& Title)
{
  m_OverrideInstructionsForResponseFormatTitle = Title;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::AddPromptHistory(const FLLMPromptBase& Prompt)
{
  m_PromptHistory.Add(Prompt);
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::RemovePromptHistory(const FLLMPromptBase& Prompt)
{
  m_PromptHistory.Remove(Prompt);
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::ClearPromptHistory()
{
  m_PromptHistory.Empty();
}

//----------------------------------------------------------------------
const TArray<FLLMPromptBase>& ULLMConnectorSubsystem::GetPromptHistory() const
{
  return m_PromptHistory;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::SetCountReservedMessages(int32 ReservedNum)
{
  m_ReservedMessages = ReservedNum;
}

//----------------------------------------------------------------------
int32 ULLMConnectorSubsystem::GetCountReservedMessages() const
{
  return m_ReservedMessages;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::RegisterCommandHandler(ULLMCommandHandlerBase* Handler)
{
  if(Handler != nullptr)
  {
    // Check if this handler is already registered
    if(!m_CommandHandlers.Contains(Handler))
    {
      m_CommandHandlers.Add(Handler);
      UE_LOG(LLM, Log, TEXT("Command handler registered: %s"), *Handler->GetClass()->GetName());
    }
  }
}

//----------------------------------------------------------------------
TArray<FLLMCommandStruct> ULLMConnectorSubsystem::GetParamsRegisterCommands() const
{
  TArray<FLLMCommandStruct> ArrParams;
  for(ULLMCommandHandlerBase* CommandIt : m_CommandHandlers)
  {
    if(CommandIt != nullptr)
    {
      ArrParams.Add(CommandIt->GetParams());
    }
  }
  return ArrParams;
}

//----------------------------------------------------------------------
FString ULLMConnectorSubsystem::GetContextCommands(const FString& InfoText /*= "Available Commands" */) const
{
  FLLMPromptNode PromptNode;
  PromptNode.ContentText = InfoText;
  for(const FLLMCommandStruct& Command : GetParamsRegisterCommands())
  {
    FLLMPromptNode CommandNode;
    CommandNode.AddChild("Target", Command.Target);
    CommandNode.AddChild("Description", Command.Description);
    if(!Command.Examples.IsEmpty())
    {
      FLLMPromptNode ExampleContext;
      for(auto& It : Command.Examples)
      {
        ExampleContext.AddChild(It);
      }
      CommandNode.AddChild("Examples", ExampleContext);
    }
    
    PromptNode.AddChild(Command.Name, CommandNode);
  }
  return PromptNode.ToString();
}

//----------------------------------------------------------------------
ULLMCommandHandlerBase* ULLMConnectorSubsystem::FindCommandHandler(const FLLMResponseBase& ResponseParams)
{
  // Find a handler that can process this command
  return FindCommandHandlerT<ULLMCommandHandlerBase>(ResponseParams);
}

//----------------------------------------------------------------------
ELLMErrorType ULLMConnectorSubsystem::TryParseParamsFromResponse(const FString& Response, FLLMResponseBase& OutParams)
{
  // Parse main response wrapper
  TSharedPtr<FJsonObject> WrapperJson;
  TSharedRef<TJsonReader<>> WrapperReader = TJsonReaderFactory<>::Create(Response);

  if(!FJsonSerializer::Deserialize(WrapperReader, WrapperJson) || !WrapperJson.IsValid())
  {
    UE_LOG(LLM, Warning, TEXT("Failed to parse wrapper JSON: %s"), *Response);
    return ELLMErrorType::InvalidResponse;
  }
  
  // Get choices array
  const TArray<TSharedPtr<FJsonValue>>* Choices;
  if(!WrapperJson->TryGetArrayField(TEXT("choices"), Choices) || Choices->Num() == 0)
  {
    UE_LOG(LLM, Warning, TEXT("No choices in response"));
    return ELLMErrorType::MissingFields;
  }
  
  // Get message content
  const TSharedPtr<FJsonObject>& FirstChoice = (*Choices)[0]->AsObject();

  FString FinishReason;
  if(FirstChoice->TryGetStringField(TEXT("finish_reason"), FinishReason)
    && (FinishReason == TEXT("length") || FinishReason == TEXT("MAX_TOKENS")))
  {
    UE_LOG(LLM, Warning, TEXT("Response was truncated"));
    return ELLMErrorType::Truncated;
  }
  
  const TSharedPtr<FJsonObject>* Message = nullptr;
  if (!FirstChoice->TryGetObjectField(TEXT("message"), Message) || !Message)
  {
    UE_LOG(LLM, Warning, TEXT("No valid message object in response"));
    return ELLMErrorType::MissingFields;
  }
  
  FString Content;
  if (!(*Message)->TryGetStringField(TEXT("content"), Content))
  {
    UE_LOG(LLM, Warning, TEXT("No content field in message"));
    return ELLMErrorType::MissingFields;
  }
  
  Content = Content.TrimStartAndEnd();

  // Check if content is wrapped in code blocks and extract JSON
  const FString JsonCodeBlockStart = TEXT("```json");
  const FString CodeBlockEnd = TEXT("```");
  if(Content.StartsWith(JsonCodeBlockStart))
  {
    int32 StartPos = JsonCodeBlockStart.Len();
    int32 EndPos = Content.Find(CodeBlockEnd, ESearchCase::IgnoreCase, ESearchDir::FromStart, StartPos);

    if(EndPos != INDEX_NONE)
    {
      // Extract only the JSON part between markers
      Content = Content.Mid(StartPos, EndPos - StartPos).TrimStartAndEnd();
    }
  }
  
  // Fix common JSON issues
  Content.ReplaceInline(TEXT(",\n}"), TEXT("\n}"));  // Remove trailing comma before closing brace
  Content.ReplaceInline(TEXT(",\r\n}"), TEXT("\r\n}"));

  // Parse the cleaned content as JSON
  TSharedPtr<FJsonObject> CommandJson;
  TSharedRef<TJsonReader<TCHAR>> CommandReader = TJsonReaderFactory<TCHAR>::Create(Content);
  
  if(!FJsonSerializer::Deserialize(CommandReader, CommandJson) || !CommandJson.IsValid())
  {
    UE_LOG(LLM, Warning, TEXT("Failed to parse command JSON, trying to fix content: %s"), *Content);
    
    // Try to fix by joining lines and removing special characters
    FString FixedContent = Content;
    TArray<FString> ContentLines;
    Content.ParseIntoArrayLines(ContentLines, false);
    if(ContentLines.Num() > 1)
    {
      FixedContent = FString::Join(ContentLines, TEXT(" "));
      CommandReader = TJsonReaderFactory<TCHAR>::Create(FixedContent);
      
      if(!FJsonSerializer::Deserialize(CommandReader, CommandJson) || !CommandJson.IsValid())
      {
        UE_LOG(LLM, Warning, TEXT("Failed to parse command JSON after fix attempt"));
        return ELLMErrorType::JsonParseError;
      }
    }
    else
    {
      return ELLMErrorType::JsonParseError;
    }
  }

  // Check if all required fields are present
  FString CommandStr, TargetStr, MessageStr;
  if(!CommandJson->TryGetStringField(TEXT("command"), CommandStr) ||
    !CommandJson->TryGetStringField(TEXT("target"), TargetStr) ||
    !CommandJson->TryGetStringField(TEXT("message"), MessageStr))
  {
    UE_LOG(LLM, Warning, TEXT("Failed to get required fields from JSON"));
    return ELLMErrorType::MissingFields;
  }

  // Extract fields into FLLMResponseBase
  OutParams.Command = CommandStr;
  OutParams.Target = TargetStr;
  OutParams.Message = MessageStr;
  
#if !UE_BUILD_SHIPPING
  FString ReasoningStr;
  if(CommandJson->TryGetStringField(TEXT("reasoning"), ReasoningStr))
  {
    OutParams.Reasoning = ReasoningStr;
  }
#endif
  
  // Get parameters array
  const TArray<TSharedPtr<FJsonValue>>* ParamsArray;
  if(CommandJson->TryGetArrayField(TEXT("parameters"), ParamsArray))
  {
    for(const auto& Param : *ParamsArray)
    {
      OutParams.Parameters.Add(Param->AsString());
    }
  }

  return ELLMErrorType::None;
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
  Super::Initialize(Collection);
  m_Settings = GetDefault<ULLMSettings>();
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::Deinitialize()
{
  m_ActiveRequests.Empty();
  Super::Deinitialize();
}

//----------------------------------------------------------------------
void ULLMConnectorSubsystem::OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
  // Remove request from active requests
  m_ActiveRequests.Remove(Request);
  
  if(!bSuccess || !Response.IsValid())
  {
    OnError.Broadcast(ELLMErrorType::InvalidAPIKey);
    return;
  }
  
  FString ResponseString = Response->GetContentAsString();
  UE_LOG(LLM, Log, TEXT("Response received: %s"), *ResponseString);
  
  // Get the response structure
  FLLMResponseBase ProcessedResponse = ProcessLLMResponse(ResponseString);
  
  // Add assistant's response to history
  FLLMPromptBase AssistantMessage(ELLMRole::Assistant, ProcessedResponse.ToString());
  AddPromptHistory(AssistantMessage);
  
  UE_LOG(LLM, Log, TEXT("%s\n"), *AssistantMessage.Content);
  
  OnResponseReceived.Broadcast(ProcessedResponse);
  OnResponseReceivedNative.Broadcast(ProcessedResponse);
  
  // Process the command and, if necessary, send the message back to the llm  
  if(OnHandleProceedCommandsResponse.IsBound())
  {
    OnHandleProceedCommandsResponse.Broadcast(ProcessedResponse);
  }
  else
  {
    TryProcessCommand(ProcessedResponse);
  }
}
