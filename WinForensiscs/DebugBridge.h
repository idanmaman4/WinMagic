#pragma once
#include <exception>
#include <DbgEng.h>
#include <concepts>


class MasterDebugBridge {
public:
    MasterDebugBridge() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            throw std::exception("Couldn't Initialize Dbg");
        }
        hr = DebugCreate(__uuidof(IDebugClient), (void**)&m_client_base);
        if (FAILED(hr)) {
            throw std::exception("Couldn't Initialize Dbg");
        }
    }

    IDebugClient* get_interface() const {
        return m_client_base;
    }
    
    ~MasterDebugBridge() {
        if(m_client_base) {
			m_client_base->Release();
		}
    }

    MasterDebugBridge(MasterDebugBridge&) = delete;
    MasterDebugBridge(MasterDebugBridge&&)= delete;
    MasterDebugBridge& operator=(MasterDebugBridge&) = delete;
    MasterDebugBridge& operator=(MasterDebugBridge&&) = delete;

private: 
    IDebugClient* m_client_base;
};



template <typename T>
concept IsDebugInterface = 
    std::derived_from<T, IDebugSymbols>    ||
    std::derived_from<T, IDebugSymbols2>   ||
    std::derived_from<T, IDebugSymbols3>   ||
    std::derived_from<T, IDebugSymbols4>   ||
    std::derived_from<T, IDebugSymbols5>   ||
    std::derived_from<T, IDebugControl>    ||
    std::derived_from<T, IDebugControl2>   || 
    std::derived_from<T, IDebugControl3>   || 
    std::derived_from<T, IDebugControl4>   || 
    std::derived_from<T, IDebugControl5>   ||
    std::derived_from<T, IDebugControl6>   ||
    std::derived_from<T, IDebugControl7>   ||
    std::derived_from<T, IDebugClient>     ||
    std::derived_from<T, IDebugClient2>    ||
    std::derived_from<T, IDebugClient3>    ||
    std::derived_from<T, IDebugClient4>    ||
    std::derived_from<T, IDebugClient5>    ||
    std::derived_from<T, IDebugClient6>    ||
    std::derived_from<T, IDebugClient7>    ||
    std::derived_from<T, IDebugClient8>    ||
    std::derived_from<T, IDebugClient9>    ||
    std::derived_from<T, IDebugDataSpaces> ||
    std::derived_from<T, IDebugDataSpaces2> ||
    std::derived_from<T, IDebugDataSpaces3> ||
    std::derived_from<T, IDebugDataSpaces4> ;


template <IsDebugInterface T>
class DebugBridge
{
public:

    DebugBridge(MasterDebugBridge& master_debug_bridge); 
    ~DebugBridge();
    T* get_interface() const;


    DebugBridge(DebugBridge&) = delete;
    DebugBridge(DebugBridge&&)= delete;
    DebugBridge& operator=(DebugBridge&) = delete;
    DebugBridge& operator=(DebugBridge&&) = delete;



private:
	T*  m_debug_bridge_interface;
    MasterDebugBridge& m_master_debug_bridge;
};


template<IsDebugInterface T>
inline DebugBridge<T>::DebugBridge(MasterDebugBridge& master_debug_bridge) : m_master_debug_bridge(master_debug_bridge)
{
    HRESULT hr = m_master_debug_bridge.get_interface()->QueryInterface(__uuidof(T), (void**)&m_debug_bridge_interface);
    if (FAILED(hr)) {
        throw std::exception("Couldn't Initialize Debug Bridge");
    }
}

template<IsDebugInterface T>
inline DebugBridge<T>::~DebugBridge()
{
	if(m_debug_bridge_interface) {
		m_debug_bridge_interface->Release();
	}
}

template <IsDebugInterface T>
T* DebugBridge<T>::get_interface() const 
{ 
    return m_debug_bridge_interface; 
}