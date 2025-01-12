/**
 * @file            configly.hpp
 * @brief           Embedded-friendly configuration manager template
 *
 * @author          Yanujz
 * @date            12/01/2025
 * @version         1.0
 */

/*
 *  MIT License
 *
 *  Copyright (c) 2025 Yanujz
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#pragma once
#include <stddef.h>
#include <string.h>

#define ARRAY_SIZE(x)    (sizeof(x) / sizeof(x[0]))

template<typename T>
class Configly
{
private:
    T m_userConfig;
    const T* m_defaultConfig; // Store default values as a constant pointer to a constant object
    void (*m_callbacks[sizeof (T)])(const void*);
          bool (*m_saveFunction)(const T &);
          bool (*m_loadUserConfig)(T &);

public:
          // Singleton instance
          static Configly<T>& instance()
    {
        static Configly<T> i;

        return i;
    }

          // Delete copy constructor and assignment operator
          Configly(Configly<T> const &) = delete;
          void operator=(Configly<T> const &) = delete;

          // Restore a single member to its default value
          template<class MemberPtr>
          void restoreDefault(MemberPtr member)
    {
        m_userConfig.*member = m_defaultConfig->*member;
    }

          // Restore all config to default values
          void restoreAllDefault()
    {
        m_userConfig = *m_defaultConfig;
    }

          // Set default config
          void setDefault(const T &defaultConfig)
    {
        m_defaultConfig = &defaultConfig;
        m_userConfig = defaultConfig; // Initialize with default values
    }

          // Get all current config
          const T& getAll() const
    {
        return m_userConfig;
    }

          // Get default config
          const T& getDefault() const
    {
        return *m_defaultConfig;
    }

          // Assign new config
          void operator=(const T &data)
    {
        m_userConfig = data;
    }

          // Set all config and trigger callbacks if enabled
          void setAll(const T &data)
    {
        m_userConfig = data;
        for(size_t i = 0; i < ARRAY_SIZE(m_callbacks); ++i)
        {
            if(m_callbacks[i])
            {
                const void* memberPtr = reinterpret_cast<const char*>(&m_userConfig) + i;
                m_callbacks[i](memberPtr);
            }
        }
    }

          // Register a callback for changes to a specific member
          template<typename MemberPtr>
          void onChange(MemberPtr member, void (*fn)(decltype(m_userConfig.*member)))
    {
        union
        {
            MemberPtr ptr;
            size_t    index;
        } converter;

        converter.ptr = member;
        m_callbacks[converter.index] = reinterpret_cast<void (*)(const void*)>(fn);
    }

          // Get the value of a specific member
          template<typename MemberPtr>
          auto get(MemberPtr member) -> decltype(m_userConfig.*member)
    {
        return m_userConfig.*member;
    }

    template<typename MemberPtr>
    auto get(MemberPtr member) const -> const decltype(m_userConfig.*member)
    {
        return m_userConfig.*member;
    }

    // Set a specific member's value
    template<typename MemberPtr, typename ValueType>
    void set(MemberPtr member, const ValueType &value)
    {
        setHelper(member, value);
    }

    // Set save function
    void setSaveFunction(bool (*fn)(const T &))
    {
        m_saveFunction = fn;
    }

    // Set load function
    void setLoadFunction(bool (*fn)(T &))
    {
        m_loadUserConfig = fn;
    }

    // Save user config
    bool save() const
    {
        if(m_saveFunction)
        {
            return m_saveFunction(m_userConfig);
        }
        return false;
    }

    // Load user config
    bool loadUserConfig()
    {
        if(m_loadUserConfig)
        {
            return m_loadUserConfig(m_userConfig);
        }
        return false;
    }

private:
    Configly()
        : m_userConfig(), m_defaultConfig(nullptr)
        , m_callbacks{}
        , m_saveFunction(nullptr), m_loadUserConfig(nullptr)
    {
    }

    ~Configly() = default;

    // Helper for setting string values
    template<typename MemberPtr>
    void setHelper(MemberPtr member, const char* value)
    {
        char*  dest = &(m_userConfig.*member)[0];
        size_t i    = 0;

        while(*value && i < sizeof(m_userConfig.*member) - 1)
        {
            *dest++ = *value++;
            ++i;
        }
        *dest = '\0';
    }

    // Helper for setting generic values
    template<typename MemberPtr, typename ValueType>
    void setHelper(MemberPtr member, const ValueType &value)
    {
        m_userConfig.*member = value;
        union
        {
            MemberPtr ptr;
            size_t    index;
        } converter;

        converter.ptr = member;

        if(m_callbacks[converter.index])
        {
            reinterpret_cast<void (*)(decltype(m_userConfig.*member))>(m_callbacks[converter.index])(m_userConfig.*member);
        }
    }
};
