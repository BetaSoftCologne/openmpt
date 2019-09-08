/*
 * BridgeWrapper.h
 * ---------------
 * Purpose: VST plugin bridge wrapper (host side)
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#ifndef NO_VST

#include "BridgeCommon.h"
#include "../common/ComponentManager.h"

OPENMPT_NAMESPACE_BEGIN

struct VSTPluginLib;

enum PluginArch : int
{
	PluginArch_unknown =        0,
	PluginArch_x86     =       32,
	PluginArch_amd64   =       64,
	PluginArch_arm     = 128 + 32,
	PluginArch_arm64   = 128 + 64,
};

std::size_t GetPluginArchPointerSize(PluginArch arch);

class ComponentPluginBridge
	: public ComponentBase
{
public:
	enum Availability
	{
		AvailabilityUnknown      =  0,
		AvailabilityOK           =  1,
		AvailabilityMissing      = -1,
		AvailabilityWrongVersion = -2,
	};
private:
	const PluginArch arch;
	mpt::PathString exeName;
	Availability availability;
protected:
	ComponentPluginBridge(PluginArch arch);
protected:
	bool DoInitialize() override;
public:
	Availability GetAvailability() const { return availability; }
	mpt::PathString GetFileName() const { return exeName; }
};

class ComponentPluginBridge_x86
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge_x86() : ComponentPluginBridge(PluginArch_x86) { }
};

class ComponentPluginBridge_amd64
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge_amd64() : ComponentPluginBridge(PluginArch_amd64) { }
};

#if defined(MPT_WITH_WINDOWS10)

class ComponentPluginBridge_arm
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge_arm() : ComponentPluginBridge(PluginArch_arm) { }
};

class ComponentPluginBridge_arm64
	: public ComponentPluginBridge
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPluginBridge_arm64() : ComponentPluginBridge(PluginArch_arm64) { }
};

#endif // MPT_WITH_WINDOWS10

class BridgeWrapper : private BridgeCommon
{
protected:
	Event m_sigAutomation;
	MappedMemory m_oldProcessMem;

	// Helper struct for keeping track of auxiliary shared memory
	struct AuxMem
	{
		std::atomic<bool> used = false;
		std::atomic<uint32> size = 0;
		MappedMemory memory;
		wchar_t name[64];
	};
	AuxMem m_auxMems[MsgStack::kStackSize];
	
	std::vector<char> m_cachedProgNames;
	std::vector<ParameterInfo> m_cachedParamInfo;
	int32 m_cachedProgNameStart = 0, m_cachedParamInfoStart = 0;
	
	bool m_isSettingProgram = false;

	Vst::ERect editRect;
	Vst::VstSpeakerArrangement speakers[2];

	ComponentHandle<ComponentPluginBridge_x86> pluginBridge_x86;
	ComponentHandle<ComponentPluginBridge_amd64> pluginBridge_amd64;
#if defined(MPT_WITH_WINDOWS10)
	ComponentHandle<ComponentPluginBridge_arm> pluginBridge_arm;
	ComponentHandle<ComponentPluginBridge_arm64> pluginBridge_arm64;
#endif // MPT_WITH_WINDOWS10

public:

	// Generic bridge exception
	class BridgeException : public std::exception
	{
	public:
		BridgeException(const char *str) : std::exception(str) { }
		BridgeException() { }
	};
	class BridgeNotFoundException : public BridgeException { };

	// Exception from bridge process
	class BridgeRemoteException
	{
	protected:
		wchar_t *str;
	public:
		BridgeRemoteException(const wchar_t *str_) : str(_wcsdup(str_)) { }
		~BridgeRemoteException() { free(str); }
		const wchar_t *what() const { return str; }
	};

public:
	static PluginArch GetNativePluginBinaryType();
	static PluginArch GetPluginBinaryType(const mpt::PathString &pluginPath);
	static bool IsPluginNative(const mpt::PathString &pluginPath) { return GetPluginBinaryType(pluginPath) == GetNativePluginBinaryType(); }
	static uint64 GetFileVersion(const WCHAR *exePath);

	static Vst::AEffect *Create(const VSTPluginLib &plugin);

protected:
	BridgeWrapper() = default;
	~BridgeWrapper();

	bool Init(const mpt::PathString &pluginPath, BridgeWrapper *sharedInstace);

	void MessageThread();

	void ParseNextMessage();
	void DispatchToHost(DispatchMsg *msg);
	bool SendToBridge(BridgeMessage &sendMsg);
	void SendAutomationQueue();
	AuxMem *GetAuxMemory(uint32 size);

	static intptr_t VSTCALLBACK DispatchToPlugin(Vst::AEffect *effect, Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);
	intptr_t DispatchToPlugin(Vst::VstOpcodeToPlugin opcode, int32 index, intptr_t value, void *ptr, float opt);
	static void VSTCALLBACK SetParameter(Vst::AEffect *effect, int32 index, float parameter);
	void SetParameter(int32 index, float parameter);
	static float VSTCALLBACK GetParameter(Vst::AEffect *effect, int32 index);
	float GetParameter(int32 index);
	static void VSTCALLBACK Process(Vst::AEffect *effect, float **inputs, float **outputs, int32 sampleFrames);
	static void VSTCALLBACK ProcessReplacing(Vst::AEffect *effect, float **inputs, float **outputs, int32 sampleFrames);
	static void VSTCALLBACK ProcessDoubleReplacing(Vst::AEffect *effect, double **inputs, double **outputs, int32 sampleFrames);

	template<typename buf_t>
	void BuildProcessBuffer(ProcessMsg::ProcessType type, int32 numInputs, int32 numOutputs, buf_t **inputs, buf_t **outputs, int32 sampleFrames);
};

OPENMPT_NAMESPACE_END

#endif // NO_VST
