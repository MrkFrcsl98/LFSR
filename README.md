# LFSR

![My Image](https://github.com/MrkFrcsl98/LFSR/blob/main/329r0iewopeiqwoewqe.jpg?raw=true)

![My Image](https://github.com/MrkFrcsl98/LFSR/blob/main/dfewrewipriewirewrewrw.jpg?raw=true)

One of the most important component within stream ciphers is the LFSR, the LFSR consists of a shift register of length "n" and a linear feedback function.
The Shift Register holds a sequence of bits, and at each clock cycle the bits are shifted to the right, and a new bit is introduced at the leftmost position,
this new bit is a linear function of the bits currently in the register. The Feedback function in an LFSR is typically a linear function, meaning it can be
represented as a polynomial over the binary field GF(2). The shift register might be initialized with a seed value, like(s0, s1, s2, s3). The state of the 
LFSR evolves over time as follows: Initial State: (s0, s1, s2, s3) Next State: (s1, s2, s3, s0 ^ s3) Here, the new bit "s0 ^ s3" is calculated based on 
the feedback polynomial. The sequence generated by an LFSR is periodic, with a maximum period of 2^n - 1 if the feedback polynomial is a primitive polynomial. 
This means that the LFSR can produce a long, non-repeating sequence of bits before it start to repeat. The choice of the feedback polynomial is crucial for 
the security and efficiency of the stream cipher. Primitive polynomials ensure that the LFSR achieves the maximum possible period. Stream cipher using LFSR 
are designed to be computationally efficient, making them suitable for hardware implementations. However the linear nature of LFSR might expose them to 
vulnerabilities, to increase security, modern stream ciphers, often implement more complex constructions, like combining multiple LFSR or using 
NLFSR(Non-Linear-Feedback-Shift-Register). An example of a more complex stream cipher is the A5/1 algorithm, which is used in GSM. A5/1 employs 3 
LFSR of different length, combined in a non-linear manner, the lengths of the LFSRs are 19, 22, 23 bits. The combining function is used to ensure 
that the output keystream is not predictable. The keystream generation process in A5/1 involves clocking the 3 LFSR in a majority rule fashion. 
Specifically, each LFSR is clocked based on the majority value of certain designated bits in the 3 registers. The majority rule mechanism introduces 
non-linearity, making it more challenging for an attacker to predict the keystream. Using single LFSR can introduce several vulnerabilities from different 
type of attacks such as correlation and algebraic attacks.

