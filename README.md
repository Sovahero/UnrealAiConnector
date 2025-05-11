![AiConnectorLogo](https://github.com/user-attachments/assets/3329e122-0b56-4f57-9bbb-637db1b91776)<br>

## AI Connector for Unreal Engine 5

### Overview
LLM Connector is a plugin that integrates Large Language Models into Unreal Engine projects. It enables connection between your game and AI models through API services such as OpenRouter and others.

The plugin provides a framework for sending user requests to LLM via HTTP and processing returned responses using JSON.
<img src="https://github.com/user-attachments/assets/5a16ad85-88c7-4f9f-b531-7bd11127c127" alt="AiTest" width="800">

## Core Concepts

### Architecture

The plugin follows a simple flow:

1. The user sends a message to the LLM through HTTP
2. The plugin processes the response through JSON and returns a structure
3. The response is displayed to the player
4. The response structure is directed to command handlers
5. Command handlers execute game logic

```json
The LLM response comes in JSON format with the following structure:
{
  "command": "command_name",
  "target": "command_target",
  "parameters": ["parameter1", "parameter2"],
  "message": "Text message for the user"
}
```
![GeneralPipeline](https://github.com/user-attachments/assets/4e746c75-c9f7-4d9c-91cc-add3db5b6925)<br>

### Command System

Commands are the primary way LLMs interact with your game:

- Each command has a name, target, and parameters
- Commands are processed by specialized handlers
- Handlers determine which game objects or systems respond to specific commands
- The system is extensible through both C++ and Blueprint

### `AiTestProject`
The repository includes a UE5.5 Blueprint test project that demonstrates a simple example of how AI controls a Character based on your instructions.

## Setup

### Plugin Installation

1. Copy `LLMConnector` to your project's `Plugins` directory (create it if it doesn't exist)
![ProjectFolder](https://github.com/user-attachments/assets/2e3b4b3d-51e9-497b-ab1b-8ece451f8a1a)<br>

2. Enable the plugin in `Edit` > `Plugins`
![ProjectPlugins](https://github.com/user-attachments/assets/89de5eb6-a141-4563-b93a-722183372a91)<br>

3. Restart the editor

### OpenRouter Setup

To use the plugin:

1. Create an account on [OpenRouter](https://openrouter.ai)<br>
2. Generate an API key
![ApiKey](https://github.com/user-attachments/assets/57171222-d908-4dc1-b05f-60923f2a03da)<br>

3. It's recommended to add funds to your account, for example $5, but you can also choose free models
4. In `Models`, select a compatible AI - Claude, GPT, etc. (Google: Gemini 2.0 Flash is set by default)
![Model](https://github.com/user-attachments/assets/9f34f3dc-d087-426f-ae71-7fb0bb59921d)<br>

5. Go to `Project Settings` > `Plugins` > `LLMSettings`
6. Enter your data in `ApiKey` and `ModelName`
![ProjectSettings](https://github.com/user-attachments/assets/38df097f-cac3-4a04-8028-fa46a6841d65)<br>

## Blueprint Quick Start

### Sending Messages

Function for sending a message and receiving a response
![SendLLMPrompt](https://github.com/user-attachments/assets/3b2677ad-0bbc-4f08-b444-c81eb618d9a2)<br>

### System Messages

To ensure the AI understands its role during the dialogue, it's recommended to add the main game context in the first messages and reserve them from deletion
![InitContext](https://github.com/user-attachments/assets/1e086f5b-52e8-4988-bed0-24fb28c4e3e6)<br>

### Creating Command Handlers

To separate logic and avoid writing everything in one place, it's recommended to use the command handler system. These handlers will intercept the response from the AI and execute the necessary logic
1. Create a Blueprint inherited from LLMCommandHandlerBase
![LLMCommandHandlerBase](https://github.com/user-attachments/assets/f3cd9ff8-6f39-491e-91a5-3a1df0f1008d)<br>
or an ActorComponent inherited from LLMCommandComponent
![LLMCommandComponent](https://github.com/user-attachments/assets/5b1506de-453f-4059-85ad-b4ad0a585bd7)<br>


3. Override the `CanExecuteCommand` and `ExecuteCommand` functions
![LLMCommandOverride](https://github.com/user-attachments/assets/08be3911-c53b-47d7-b89f-c59c425e99b7)<br>

4. Configure the command handler parameters and register it with LLMConnector
![LLMCommandHandlerBaseParams](https://github.com/user-attachments/assets/97df44bc-39af-4a0a-9949-40142b2cae63)<br>
For ActorComponent, attach it to an Actor and set the flag
![LLMCommandComponentParams](https://github.com/user-attachments/assets/59ecea07-f7ce-41b8-8c3e-090deacae76b)<br>


## C++ Quick Start

### Initialization
```cpp
ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(this);
if(LLMConnector)
{
  LLMConnector->OnResponseReceived.AddUniqueDynamic(this, &ThisClass::OnReceived);
  LLMConnector->OnError.AddUniqueDynamic(this, &ThisClass::OnError);
}
```
### Sending Messages
```cpp
// Send a message to the LLM
ULLMConnectorSubsystem* LLMConnector = ULLMConnectorSubsystem::GetLLMConnector(this);
if (LLMConnector && LLMConnector->CanSendLLMPrompt())
{
  LLMConnector->SendLLMPrompt(TEXT("Move the character left for 5 seconds"), ELLMRole::User);
}
```

### System Messages
```cpp
// Send only if history is empty
if(m_LLMConnector == nullptr || m_LLMConnector->GetPromptHistory().Num() > 0)
{
  return false;
}

// Creating a root context map
m_LLMConnector->AddPromptHistory(FLLMPromptBase(ELLMRole::System, TEXT("You are an AI assistant in a game world.")));
m_LLMConnector->AddPromptHistory(FLLMPromptBase(ELLMRole::System, m_LLMConnector->GetContextCommands()));
m_LLMConnector->SetCountReservedMessages(m_LLMConnector->GetPromptHistory().Num());
```

### Creating a Command Handler

```cpp
// MyCommandHandler.h
#pragma once

#include "CoreMinimal.h"
#include "LLMCommandHandler.h"

#include "LLMItemCommandBase.generated.h"

/**
 * Your CommandHandler
 */
UCLASS()
class UMyCommandHandler : public ULLMCommandHandlerBase
{
  GENERATED_BODY()
public:
  virtual bool CanExecuteCommand(const FLLMResponseBase& ResponseParams) override
  {
    if(GetParams().Name.ToLower() != ResponseParams.Command.ToLower())
    {
      return false;
    }
    return ResponseParams.Parameters.Num() > 0;
  }
  virtual FString ExecuteCommand(const FLLMResponseBase& ResponseParams) override
  {
    // Based on the LLM response, perform actions using values from these fields
    ResponseParams.Parameters
    ResponseParams.Target

    // Return a response to LLM if you want to get another answer (Useful for game database interaction commands)
    return TEXT("Requested data");

    // Or specify an empty string if it's sufficient to just execute this command
    return TEXT("");
  }
};
```

### Registering Command Handlers

```cpp
ULLMCommandHandlerBase* NewHandler = NewObject<UMyCommandHandler>(this);

FLLMCommandStruct Params;
Params.Name = TEXT("move");
Params.Description = TEXT("Command to move the character in specified direction for a certain amount of time");
Params.Target = TEXT("character");
Params.Examples = { TEXT("{command: 'move', target: 'character', parameters: ['forward#4'], message: 'Going forward 4 seconds.'}") };
NewHandler->InitWithParams(Params);

m_LLMConnector->RegisterCommandHandler(NewHandler);
```

### Context Description Structures
For convenient description of the game world and parameters, use the `FLLMPromptNode` and `FContextDescription` structures
```cpp
// Helps create internal nesting for storing and sending text

// In your Blueprint Manager
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters Context Description")
TMap<EGameLevelType, FContextDescription> GameLevelContextDesc;
...
FLLMPromptNode RootNode;
RootNode.ContentText = TEXT("Description of game types");
FContextDescription::ConvertContextMapToNode(GameLevelContextDesc, RootNode);

FString GameLevelContext = RootNode.ToString();
```

## Implementation Considerations

- Communication with LLMs over HTTP is similar to a ping-pong game with delays from 1 second. Currently, it's not possible to receive responses on-the-fly
- When possible, use paid models; free models are not as intelligent and have message quotas
- Some models cannot produce responses in JSON format, for example, DeepSeek
- Each message consumes tokens. The more tokens used, the more expensive the request. On average, 1 English character uses ~0.25 tokens (4 characters = 1 token), and characters of other languages may use up to ~0.3 tokens (approximately 3 characters of another language = 1 token). It's recommended to describe the game context in English to optimize costs, while user communication can be conducted in any language
- When you send a message each time, the entire history of previous messages is also sent; by default, there's a limit of 20 messages, after which the oldest messages will be deleted
- Break down your game context into message history; this helps the LLM understand better
- Provide clear instructions in system prompts about available commands
- Include examples of proper command formatting
- Store API keys in a secure place and don't disclose them
