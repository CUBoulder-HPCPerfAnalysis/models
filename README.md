# Automated performance modeling

For this topic, we're starting from [Calotoiu, Hoefler, Poke, and Wolf, *Using Automated Performance Modeling to Find Scalability Bugs in Complex Codes*, SC2013](http://unixer.de/publications/img/calotoiu-scal-bugs-sc13.pdf).
As an alternative to the cross-validation used in the above paper, we're considering fitting performance models using [l1-regularized regression](https://en.wikipedia.org/wiki/Least_squares#Lasso_method).
This repository will be used for model fitting using [cvxopt](http://cvxopt.org), a high-level Python interface for convex optimization.

## Examples
* src/snes/examples/tutorials/ex48.c - 3D Hydrostatic Ice Flow Q_1 elements
* src/ksp/ksp/examples/tutorials/ex49.c - 2D Elasticity Q_1 elements
* [HPGMG](https://hpgmg.org) - 3D Poisson on deformed grid, Q_2 elements

## Useful run-time options
* `-pc_mg_log` - profiling information by level

# Running on Janus

First log into a compilation node.

    $ ssh you1234@login.rc.colorado.edu
    <4-digit PIN><6-digit cryptokey code>      # password
    $ ssh janus-compile4                       # preferred compilation environment

Now load the modules necessary to submit jobs.

    $ module load slurm gcc/gcc-4.9.2
    $ export PETSC_DIR=/home/jeka2967/petsc PETSC_ARCH=ompi-gcc49-optg

Do *not* load an openmpi module (they don't have a module for Open MPI 1.8.4 even thought it has been build; version 1.8.3 and earlier have buggy attribute caching).
Instead, we'll just use the full path to the proper installation.
See `${PETSC_DIR}/${PETSC_ARCH}/lib/petsc-conf/reconfigure*.py` for the arguments I used to build PETSc.

Jobs are submitted using SLURM.  See [RC Submitting MPI Jobs](https://www.rc.colorado.edu/support/userguidesubmittingmpijobs) for generic instructions.
As an example, I'm using the following script `runex48.sh`

    #!/bin/sh
    
    #SBATCH --qos=janus
    #SBATCH -N 16
    #SBATCH --ntasks-per-node=12
    #SBATCH --time=00:05:00
    
    export PATH=/curc/tools/x_86_64/rh6/openmpi/1.8.4/gcc/4.9.2/bin:${PATH}
    mpiexec ./ex48 -M 24 -P 32 -snes_max_linear_solve_fail 50 -ksp_type cg -pc_type jacobi -ksp_max_it 3 -snes_converged_reason -snes_monitor -log_summary
    mpiexec ./ex48 -M 24 -P 32 -snes_max_linear_solve_fail 50 -ksp_type cg -pc_type jacobi -ksp_max_it 3 -snes_converged_reason -snes_monitor -log_summary -matstash_bts

and submitting it with

    $ qsub runex48.sh

# Model-Specific Registers

Model-Specific Registers can be used to change hardware characteristics including prefetch policy.
The cryptic register names (like `0x1a0`) are described in Intel's volume [3C: Systems Programming Guide](https://www-ssl.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3c-part-3-manual.pdf).
These registers can be manipulated on Linux using `rdmsr` and `wrmsr` from [msr-tools](https://01.org/msr-tools/overview).
For [example](http://lists.mcs.anl.gov/pipermail/petsc-users/2010-January/005840.html),

    # rdmsr -p 1 0x1a0
    1364970489

and flipping bit 9 (counting from 0 on the right) disables a type of hardware prefetch, so

    # wrmsr -p 1 0x1a0 0x1364970689

turns off prefetch and

    # wrmsr -p 1 0x1a0 0x1364970489

turns it back on again.

# The `cache-interfere` program

The `cache-interfere` program in this repository can be used to occupy some part of cache by repeatedly touching lines.
Compile with

    make CC=gcc CFLAGS='-std=c99 -O2 -Wall -Wextra' cache-interfere

It works by `fork`ing and `exec`uting your application from the child while spinning in the parent.

    Usage: ./cache-interfere [-s set_size] [-c cycles] app [args]

The `set_size` is the number of bytes to keep in cache and `cycles` is a crude estimate of the number of clock cycles to spin before retouching the set.
