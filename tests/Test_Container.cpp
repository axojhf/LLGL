/*
 * Test_Container.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include <LLGL/LLGL.h>
#include <LLGL/Container/SmallVector.h>
#include <LLGL/Container/Strings.h>
#include <iostream>
#include <sstream>
#include <vector>


class StopwatchScope
{

    public:

        StopwatchScope(const char* name) :
            name_      { name                },
            startTick_ { LLGL::Timer::Tick() }
        {
        }

        ~StopwatchScope()
        {
            auto endTick = LLGL::Timer::Tick();
            auto elapsedTime = (static_cast<double>(endTick - startTick_) / static_cast<double>(LLGL::Timer::Frequency())) * 1000.0;
            printf("%s: %fms\n", name_, elapsedTime);
        }

    private:

        const char*     name_;
        std::uint64_t   startTick_;

};

int main()
{
    try
    {
        LLGL::UTF8String sa, sb, sc, sd;

        std::wstring sc_orig = L"\u3053\u3093\u306B\u3061\u306F\u4E16\u754C\u3002";
        sc = sc_orig.c_str();
        LLGL::SmallVector<wchar_t> sc_array = sc.to_utf16();
        std::wstring sc_back = sc_array.data();

        sa = "Hello";
        sb = L"World";
        sd = sa + " " + sb + "\n" + sc;

        std::string s = sd.c_str();

        LLGL::SmallVector<wchar_t> wsd = sd.to_utf16();
        std::wstring ws = wsd.data();

        std::cout << s.c_str() << std::endl;
        std::wcout << ws.c_str() << std::endl;

        struct CustomGrowth
        {
            static inline std::size_t capacity(std::size_t size)
            {
                return size + size / 2;
            }
        };

        for (int n = 0; n < 10; ++n)
        {
            {
                StopwatchScope scope{ "LLGL::SmallVector<int>::push_back(0 .. 10000000)" };

                LLGL::SmallVector<int> l1;
                l1.reserve(10000000);
                for (int i = 0; i < 10000000; ++i)
                    l1.push_back(i);
            }

            {
                StopwatchScope scope{ "std::vector<int>::push_back(0 .. 10000000)" };

                std::vector<int> l2;
                l2.push_back(1);
                l2.reserve(10000000);
                for (int i = 0; i < 10000000; ++i)
                    l2.push_back(i);
            }
        }


        #ifdef _WIN32
        system("pause");
        #endif
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
