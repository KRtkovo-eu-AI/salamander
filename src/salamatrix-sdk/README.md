# Salamatrix SDK

This directory contains host-independent contracts shared by Salamatrix,
Salamatrix Studio, generators, and standalone preview tooling. Code under this
directory must not depend on `salamand.exe`, plug-in globals, or a running Open
Salamander instance.

The first contract is the versioned dialog design document. Native UI runtime
extraction and the standalone preview host will build on this boundary.
