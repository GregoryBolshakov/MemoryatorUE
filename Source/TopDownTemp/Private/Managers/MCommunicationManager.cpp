#include "MCommunicationManager.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "Async/Async.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Characters/MCharacter.h"
#include "GenericTeamAgentInterface.h"

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
	FString address = TEXT("127.0.0.1");
	int32 port = 12345;
	FIPv4Address ip;
	FIPv4Address::Parse(address, ip);
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.Value);
	addr->SetPort(port);

	FSocket* socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("PythonSocket"), false);

	bool connected = socket->Connect(*addr);

	if (connected)
	{
		// Start reading data in a separate thread
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [socket, this]()
		{
			ReadDataFromSocket(socket);
		});
	}
	else
	{
		// Handle connection failure
		UE_LOG(LogTemp, Error, TEXT("Failed to connect to Python server"));
	}
}

void AMCommunicationManager::ReadDataFromSocket(FSocket* Socket)
{
	// Endless loop for reading from socket
	while (Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(1000.f)))
	{
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

	// Clean up
	Socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
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

void AMCommunicationManager::BeginPlay()
{
	Super::BeginPlay();

	FGenericTeamId::SetAttitudeSolver(CustomTeamAttitudeSolver);
}
