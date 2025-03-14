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

#ifndef LFSR_BIT_INIT 
#define LFSR_BIT_INIT 0x0af0 // (2800)
#endif

#define __improved_exec __attribute__((hot, always_inline, no_stack_protector))

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

__improved_exec inline static void registerFeedback(const std::uint8_t _f, const std::size_t _seqSZ,
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
    if (StateMap.size() > 0x0)
    {
        for (auto &[dx, state] : StateMap)
        {
            while (state.block_stream.size() % 0x8 != 0x0)
            {
                state.block_stream.push_back(0);
            }
        }
        for (std::size_t s{0x0}; s < StateMap.size(); ++s)
        {
            for (auto &[k, s] : StateMap)
            {
                for (std::size_t c{0x0}; c < s.block_stream.size(); c += 0x8)
                {
                    std::string str("");
                    for (int i = 0x0; i < 0x8; ++i)
                    {
                        str += std::to_string(s.block_stream[i + c]);
                    }
                    std::bitset<0x8> bits(str);
                    output_stream += bits.to_ullong();
                }
            }
        }
    }
};

__improved_exec inline static void registerClockSequence(const std::size_t _seqSZ, const std::vector<tBit> &_sequence)
{
    if (StateMap.find(_seqSZ) != StateMap.end())
    {
        StateMap[_seqSZ].clock_sequence.push_back(_sequence);
    }
};

__improved_exec inline static const bool collisionDetection(const std::vector<tBit>& _seq)  {
    bool is = false;
    try{
        if(StateMap.size() > 0x0) [[likely]] {
            for(const auto&[k, v]: StateMap){
                for(int i = 0x0; i < v.block_stream.size() && i < _seq.size(); ++i){
                    if(_seq[i] != v.block_stream[i]) {
                        is = false;
                        break;
                    }else if(i == _seq.size() - 0x1) [[unlikely]] {
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

int main(int argc, char** argv)
{
    try
    {
        std::size_t collisions{0};
        constexpr std::uint16_t initial_state = LFSR_BIT_INIT;
        constexpr std::uint8_t INITIAL_SEED_SIZE{sizeof(initial_state) * 0x8};
        std::vector<std::uint64_t> bit_sequence(INITIAL_SEED_SIZE, 0x0);
        std::bitset<INITIAL_SEED_SIZE> initBits(initial_state);
        for (int i = 0x0; i < bit_sequence.size(); ++i)
        {
            bit_sequence[i] = initBits[i];
        }
        std::uint16_t SEQ_SZ = bit_sequence.size();
        std::uint8_t tap_A = 0x0, tap_B = SEQ_SZ - 0x1;

        const std::uint64_t CYCLE_THRESHOLD = static_cast<std::uint64_t>(std::pow(0x2, bit_sequence.size()));

        std::cout << "Using initial state: " << (int)initial_state << "\n";

        // try for all possible intervals available
        for (std::uint32_t cycle{0x0}; cycle <= CYCLE_THRESHOLD; ++cycle)
        {

            std::this_thread::sleep_for(std::chrono::milliseconds(0x1));
            std::cout << cycle << "/" << CYCLE_THRESHOLD << ") " << (int)tap_A << " " << (int)tap_B << " = ";
            for(int i = 0x0; i < SEQ_SZ; ++i) {
                std::cout << (int)bit_sequence[i];
            }
            std::cout << "\n";

            // collision detection block
            if(collisionDetection(bit_sequence)) {
                ++collisions;
                std::cout << "Collision Detected: " << cycle << "/" << CYCLE_THRESHOLD << ") " << (int)tap_A << " " << (int)tap_B << " = ";
                for(int i{0x0}; i < bit_sequence.size(); ++i){
                    std::cout << bit_sequence[i];
                }
                std::cout << "\n";
                //std::this_thread::sleep_for(std::chrono::seconds(3));
            }

            registerClockSequence(SEQ_SZ, bit_sequence); // register current clock sequence
            const std::uint8_t feedback =
                bit_sequence[tap_A] ^ bit_sequence[tap_B]; // save feedback state

            // register right shift
            for (std::uint16_t i{SEQ_SZ}; i >= 0x0;)
            {
                if (--i <= 0x0)
                {
                    break;
                }
                else
                {
                    bit_sequence[i] = bit_sequence[i - 0x1];
                }
            }

            bit_sequence[0] = feedback;                       // update register
            registerFeedback(feedback, SEQ_SZ, tap_A, tap_B); // register feedback

            // switch tap positions
            if (cycle >= CYCLE_THRESHOLD - 0x1)
            {
                bit_sequence.clear();
                std::cout << "Old State: " << initBits.to_string() << "\n";
                initBits |= (tap_A + tap_B);
                initBits <<= 1;
                std::cout << "Using new State: " << initBits.to_string() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (int i{0x0}; i < bit_sequence.size(); ++i)
                {
                    bit_sequence[i] = initBits[i];
                }
                if (tap_A < SEQ_SZ - 1)
                {
                    if(++tap_A % tap_B == 0x0) {
                        --tap_B;
                    } 
                }
                else if (tap_B > 0x0)
                {
                    --tap_B;
                }
                else
                {
                    break;
                }
                cycle = 0x0;
            }
        }

        finalizeSequenceStateStream();
        std::cout << "Feedback Map Size = " << StateMap.size() << "\n";

        std::cout << "Calculated Stream Output: " << output_stream << "\n";

        std::cout << "Total Collisions: " << collisions << "\n";
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
    return 0x0;
}
