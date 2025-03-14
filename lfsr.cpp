#include <algorithm>
#include <bitset>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef std::uint64_t tBit;

typedef struct
{
    std::vector<tBit> block_stream{};
    std::vector<std::vector<tBit>> clock_sequence{};
    std::uint8_t tA{};
    std::uint8_t tB{};
} SequenceStates;

std::unordered_map<std::size_t, SequenceStates> StateMap;

std::uint64_t output_stream{0};

__attribute__((hot, always_inline)) inline static void registerFeedback(const std::uint8_t _f, const std::size_t _seqSZ,
                             const std::uint8_t t_a, const std::uint8_t t_b)
{
    if (StateMap.find(_seqSZ) != StateMap.end())
    {
        StateMap[_seqSZ].block_stream.push_back(_f);
    }
    else
    {
        SequenceStates SS;
        SS.tA = t_a;
        SS.tB = t_b;
        SS.block_stream.push_back(_f);
        StateMap.insert({_seqSZ, SS});
    }
};

__attribute__((cold)) static void finalizeSequenceStateStream()
{
    if (StateMap.size() > 0)
    {
        for (auto &[dx, state] : StateMap)
        {
            while (state.block_stream.size() % 8 != 0)
            {
                state.block_stream.push_back(0);
            }
        }
        for (std::size_t s{0}; s < StateMap.size(); ++s)
        {
            for (auto &[k, s] : StateMap)
            {
                for (std::size_t c{0}; c < s.block_stream.size(); c += 8)
                {
                    std::string str("");
                    for (int i = 0; i < 8; ++i)
                    {
                        str += std::to_string(s.block_stream[i + c]);
                    }
                    std::bitset<8> bits(str);
                    output_stream += bits.to_ullong();
                }
            }
        }
    }
};

__attribute__((hot, always_inline, no_stack_protector)) inline static void registerClockSequence(const std::size_t _seqSZ, const std::vector<tBit> &_sequence)
{
    if (StateMap.find(_seqSZ) != StateMap.end())
    {
        StateMap[_seqSZ].clock_sequence.push_back(_sequence);
    }
};

__attribute__((hot, always_inline)) inline static const bool collisionDetection(const std::vector<tBit>& _seq)  {
    bool is = false;
    try{
        if(StateMap.size() > 0) [[likely]] {
            for(auto&[k, v]: StateMap){
                for(int i = 0; i < v.block_stream.size() && i < _seq.size(); ++i){
                    if(_seq[i] != v.block_stream[i]) {
                        is = false;
                        break;
                    }else if(i == _seq.size() - 1) [[unlikely]] {
                        is = true;
                    }
                }
            }
        }
    }catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return is;
};

int main()
{
    try
    {
        constexpr std::uint16_t initial_state = 4238u;
        constexpr std::uint8_t INITIAL_SEED_SIZE{sizeof(initial_state) * 8};
        std::vector<std::uint64_t> bit_sequence(INITIAL_SEED_SIZE, 0);
        std::bitset<INITIAL_SEED_SIZE> initBits(initial_state);
        for (int i = 0; i < bit_sequence.size(); ++i)
        {
            bit_sequence[i] = initBits[i];
        }
        std::uint16_t SEQ_SZ = bit_sequence.size();
        std::uint8_t tap_A = 0, tap_B = SEQ_SZ - 1;

        const std::uint64_t CYCLE_THRESHOLD = static_cast<std::uint64_t>(std::pow(2, bit_sequence.size()) * 129);

        std::cout << "Using initial state: " << (int)initial_state << "\n";

        // try for all possible intervals available
        for (std::uint32_t cycle{0}; cycle <= CYCLE_THRESHOLD; ++cycle)
        {

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::cout << cycle << "/" << CYCLE_THRESHOLD << ") " << (int)tap_A << " " << (int)tap_B << " = ";
            for(int i = 0; i < SEQ_SZ; ++i) {
                std::cout << (int)bit_sequence[i];
            }
            std::cout << "\n";

            // collision detection block
            if(collisionDetection(bit_sequence)) {
                std::cout << "Collision Detected: " << cycle << "/" << CYCLE_THRESHOLD << ") " << (int)tap_A << " " << (int)tap_B << " = ";
                for(int i = 0; i < bit_sequence.size(); ++i){
                    std::cout << bit_sequence[i];
                }
                std::cout << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }

            registerClockSequence(SEQ_SZ, bit_sequence); // register current clock sequence
            const std::uint8_t feedback =
                bit_sequence[tap_A] ^ bit_sequence[tap_B]; // save feedback state

            // register right shift
            for (std::uint16_t i{SEQ_SZ}; i >= 0;)
            {
                if (--i <= 0)
                {
                    break;
                }
                else
                {
                    bit_sequence[i] = bit_sequence[i - 1];
                }
            }

            bit_sequence[0] = feedback;                       // update register
            registerFeedback(feedback, SEQ_SZ, tap_A, tap_B); // register feedback

            // switch tap positions
            if (cycle >= CYCLE_THRESHOLD - 1)
            {
                bit_sequence.clear();
                std::cout << "Old State: " << initBits.to_string() << "\n";
                initBits |= (tap_A + tap_B);
                initBits <<= 1;
                std::cout << "Using new State: " << initBits.to_string() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(4));
                for (int i = 0; i < bit_sequence.size(); ++i)
                {
                    bit_sequence[i] = initBits[i];
                }
                if (tap_A < SEQ_SZ - 1)
                {
                    if(++tap_A % tap_B == 0) {
                        --tap_B;
                    } 
                }
                else if (tap_B > 0)
                {
                    --tap_B;
                }
                else
                {
                    break;
                }
                cycle = 0;
            }
        }

        finalizeSequenceStateStream();
        std::cout << "Feedback Map Size = " << StateMap.size() << "\n";

        std::cout << "Calculated Stream Output: " << output_stream << "\n";
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid Argument Error: " << e.what() << "\n";
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Runtime Error: " << e.what() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
