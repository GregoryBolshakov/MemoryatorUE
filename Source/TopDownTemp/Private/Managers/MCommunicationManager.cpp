#include "MCommunicationManager.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "Async/Async.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Characters/MCharacter.h"
#include "GenericTeamAgentInterface.h"
#include "Helpers/MScopeLock.h"
#include "Misc/ByteSwap.h"

namespace
{
	ETeamAttitude::Type CustomTeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
	{
		// Normalize the order of A and B to ensure consistency
		if (A.GetId() > B.GetId())
		{
			Swap(A, B);
		}

		if (A == GetTeamIdByEnum(EMTeamID::Nightmares) && B == GetTeamIdByEnum(EMTeamID::Witches))
		{
			return ETeamAttitude::Neutral;
		}

		// TODO: Put other cross-faction attitudes here

		return A != B ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
	}
}

void AMCommunicationManager::ConnectToPythonServer()
{
	FString Address = TEXT("127.0.0.1");
	int32 Port = 12345;
	FIPv4Address IP;
	FIPv4Address::Parse(Address, IP);
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetIp(IP.Value);
	Addr->SetPort(Port);

	{
		FMScopeSpinLock Lock(&SocketLock);
		Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("PythonSocket"), false);
		Connected = Socket->Connect(*Addr);
		if (Connected)
		{
			// Start reading data in a separate thread
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]
			{
				ReadDataFromSocket();
			});
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to connect to Python server"));
		}
	}
}

void AMCommunicationManager::DisconnectFromPythonServer()
{
	FString CloseCommand = TEXT("{\"command\": \"close\"}");
	check(SendJsonMessage(CloseCommand));
	{
		FMScopeSpinLock Lock(&SocketLock);
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
}

void BusyWait(FTimespan Duration)
{
	const double StartTime = FPlatformTime::Seconds();
	const double EndTime = StartTime + Duration.GetTotalSeconds();

	while (FPlatformTime::Seconds() < EndTime)
	{
		// Busy-wait: no thread sleep or yield here.
		FPlatformMisc::MemoryBarrier(); // Optional, helps prevent compiler optimizations.
	}
}

void AMCommunicationManager::ReadDataFromSocket()
{
	auto test = 1;
	// Endless loop for reading from socket
	while (true)
	{
		FMScopeSpinLock Lock(&SocketLock);
		BusyWait(FTimespan::FromMilliseconds(100));
		if (!Socket || Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
		{
			break;
		}
		if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(100)))
		{
			continue; // No data to read, continue waiting
		}
		int32 bytesRead = 0;
		uint8 data[1024];
		if (Socket->Recv(data, sizeof(data), bytesRead))
		{
			if (bytesRead > 0)
			{
				FString ReceivedString = FString(FUTF8ToTCHAR(reinterpret_cast<const char*>(data), bytesRead));
				CutPreviousMessages(ReceivedString);

				UE_LOG(LogTemp, Log, TEXT("Received Text: %s"), *ReceivedString);
			}
		}
		else
		{
			break;
		}
	}
}

void AMCommunicationManager::CutPreviousMessages(FString& Message)
{
	// The substring to search for
	const FString SubstringToFind = TEXT("<end>");

	// Find the last occurrence of the substring
	int32 LastIndex = Message.Find(SubstringToFind, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (LastIndex == INDEX_NONE)
	{
		// No occurrence of "<end>" found, return the original string
		return;
	}

	// Find the second-to-last occurrence of the substring
	int32 SecondLastIndex = Message.Find(SubstringToFind, ESearchCase::IgnoreCase, ESearchDir::FromEnd, LastIndex - 1);
	if (SecondLastIndex == INDEX_NONE)
	{
		// Only one occurrence of "<end>" found, return the original string
		return;
	}

	// Return the string starting from the second-to-last "<end>"
	int32 ChopIndex = SecondLastIndex + SubstringToFind.Len();
	Message = Message.RightChop(ChopIndex);
}

FString AMCommunicationManager::GenerateMessagesJson(const AMCharacter* Character)
{
	// Create the messages json array
	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	/*TSharedPtr<FJsonObject> Message1 = MakeShareable(new FJsonObject);
	Message1->SetStringField(TEXT("role"), TEXT("system"));
	Message1->SetStringField(TEXT("content"), TEXT("You are a decision maker model for an NPC character in an RPG game. Your personality is a peasant man. You sell flowers. User is a single player in this game. All user input comes from the player. Your attitude toward him is mediocre. You don't necessarily have to do as he says, you have your own judgement."));
	MessagesArray.Add(MakeShareable(new FJsonValueObject(Message1)));

	TSharedPtr<FJsonObject> Message2 = MakeShareable(new FJsonObject);
	Message2->SetStringField(TEXT("role"), TEXT("user"));
	Message2->SetStringField(TEXT("content"), TEXT("Hey everybody come closer!"));
	MessagesArray.Add(MakeShareable(new FJsonValueObject(Message2)));

	TSharedPtr<FJsonObject> Message3 = MakeShareable(new FJsonObject);
	Message3->SetStringField(TEXT("role"), TEXT("system"));
	Message3->SetStringField(TEXT("content"), TEXT("You must choose one: Come closer; Ignore him; Insult him. Say nothing else but your choice."));
	MessagesArray.Add(MakeShareable(new FJsonValueObject(Message3)));*/

	TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject);
	Message->SetStringField(TEXT("role"), TEXT("system"));
	Message->SetStringField(TEXT("content"), TEXT("Say only \"Hi!\""));
	MessagesArray.Add(MakeShareable(new FJsonValueObject(Message)));

	// Create characterUid json object
	TSharedPtr<FJsonObject> CharacterUid = MakeShareable(new FJsonObject);
	CharacterUid->SetNumberField(TEXT("LaunchId"), Character->GetUid().LaunchId);
	CharacterUid->SetNumberField(TEXT("ObjectId"), Character->GetUid().ObjectId);

	// Create the root object
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField(TEXT("messages"), MessagesArray);
	RootObject->SetObjectField(TEXT("characterUid"), CharacterUid);

	// Convert to JSON string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	bool IsSerialized = FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	check(IsSerialized);
	Writer->Close();

	return OutputString;
}

bool AMCommunicationManager::SendJsonMessage(const FString& JsonMessage)
{
	// Convert the message to UTF-8 format
	FTCHARToUTF8 Converter(*JsonMessage);
	int32 DataSize = Converter.Length();
	const uint8* DataPtr = reinterpret_cast<const uint8*>(Converter.Get());

	// Convert DataSize to network byte order
	uint32 NetworkDataSize = NETWORK_ORDER32(DataSize);

	// Send the NetworkDataSize (4 bytes)
	{
		int32 BytesSent = 0;
		FMScopeSpinLock Lock(&SocketLock);
		if (!Socket)
		{
			check(false);
			return false;
		}
		bool bSuccess = Socket->Send(reinterpret_cast<const uint8*>(&NetworkDataSize), sizeof(int32), BytesSent);
		if (!bSuccess || BytesSent != sizeof(int32))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to send data size."));
			return false;
		}

		// Send the actual data
		int32 TotalBytesSent = 0;
		while (TotalBytesSent < DataSize)
		{
			int32 BytesToSend = DataSize - TotalBytesSent;
			int32 BytesThisSend = 0;
			bSuccess = Socket->Send(DataPtr + TotalBytesSent, BytesToSend, BytesThisSend);
			if (!bSuccess || BytesThisSend <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to send message."));
				return false;
			}
			TotalBytesSent += BytesThisSend;
		}
	}

	return true;
}

void AMCommunicationManager::SendMessagesToServer(const AMCharacter* Character)
{
	FString MessagesJson = GenerateMessagesJson(Character);

	check(SendJsonMessage(MessagesJson));
}

void AMCommunicationManager::BeginPlay()
{
	Super::BeginPlay();

	FGenericTeamId::SetAttitudeSolver(CustomTeamAttitudeSolver);

	ConnectToPythonServer();
}

void AMCommunicationManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisconnectFromPythonServer();
	Super::EndPlay(EndPlayReason);
}
