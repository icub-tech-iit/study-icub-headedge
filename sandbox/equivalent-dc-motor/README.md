Equivalent DC model of a PMSM motor
===================================

Considering a PMSM motor controlled via FOC (i.e., the direct current can be neglected), the equivalence with a corresponding DC motor can be enforced by imposing the following relations:
- The armature resistance of the DC model is equal to the stator resistance per phase $`R_s`$.
- The armature inductance of the DC model is equal to the stator q-axis inductance $`L_q`$[^1].
- $`K_t^{DC} = \frac{3}{2} \cdot K_t^{PMSM}`$, where $`K_t`$ is the torque constant.
- A portion of the back-emf equal to $`\frac{1}{2} \cdot K_e^{PMSM}`$ is compensated internally, where $`K_e`$ (V/rad/s) is the back-emf constant, which is equal in magnitude to $`K_t`$ (Nm/A), guaranteeing the the units are correct.

### The Equivalent DC Model
![graph](./assets/equivalent-model.png)

### Results of Speed + Current Control Equivalence 
![graph](./assets/graph.png)

### Resources
- [PMSM dynamic model](https://www.mathworks.com/help/releases/R2024a/sps/ref/pmsm.html)
- [Three-Phase PMSM Drive](https://www.mathworks.com/help/releases/R2024a/sps/ug/three-phase-pmsm-drive.html)

[^1]: It may be non trivial to find the value of the q-axis inductance in the datasheet. At any rate, thanks to the closed-loop, the equivalence holds fairly robust if we vary the inductance in the range [0.6, 2] * nominal.