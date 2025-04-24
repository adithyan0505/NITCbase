
# NITCbase

This repository contains my implementation of [NITCbase](https://nitcbase.github.io/) — a rudimentary, single-user Relational Database Management System (RDBMS) developed in C++ as part of a university-level laboratory course.

It follows an [eight-layer](https://nitcbase.github.io/docs/Design/Architecture) object-oriented [system design](https://nitcbase.github.io/docs/Design/DesignDiagram) and implements core relational database features, including table creation and deletion, record insertion, selection queries, and indexing using B+ Trees.

## Table of Contents
- [Stages](#stages)
- [Functionalities](#functionalities)
- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
  - [Installation](#installation)
  - [Running NITCbase](#running-nitcbase)
- [Documentation](#documentation)
- [License](#license)
- [To Future Implementors](#to-future-implementors)

## Stages
- Development of this project was carried out in multiple stages, following the NITCbase [Roadmap](https://nitcbase.github.io/docs/Roadmap/).
- To view stage-wise progress, refer the respective branches.
> **Note:** Older branches may contain bugs that have been resolved by stage-12. All fixes have been tested and merged into the main branch.

## Functionalities

NITCBase provides a simple command-line interface for executing database operations. It supports elementary SQL queries such as `CREATE`, `DROP`, `ALTER`, `INSERT`, `SELECT`, `PROJECT`, and `JOIN`(equi-join), as well as B+ Tree-based indexing operations like `CREATE INDEX` and `DROP INDEX`. Currently, NITCBase does not support concurrency.

> **Note:** For the complete list of supported SQL queries, refer to [this](https://nitcbase.github.io/docs/User%20Interface%20Commands/).

## Prerequisites

Before running NITCBase, ensure that you have the following prerequisites installed on your system:

- A Linux-based operating system (or an equivalent [Docker](https://docs.docker.com/) container instance)
- C/C++ compiler (such as gcc/g++)
- Git

> **Note:** This implementation was built and tested on an Ubuntu container instance via Docker on macOS.

## Getting Started

Follow these steps to set up and run NITCBase on your system:

### Installation

1. Clone the NITCBase repository to your local machine:

   ```bash
   git clone https://github.com/adithyan0505/NITCbase
   ```

2. Navigate to the project directory:

   ```bash
   cd NITCBase/mynitcbase
   ```

3. Build the NITCBase application using the `make` command:

   ```bash
   make
   ```

4. In case you run into any issues while creating builds, install the necessary packages.

### Running NITCBase

Once you have successfully built NITCBase, you can run it with the following command:

```bash
./nitcbase
```

## Documentation

For detailed information on the architecture, methodology, design and roadmap, please refer to the NITCBase [Documentation](https://nitcbase.github.io/).

## License 
NITCbase is licensed under the [MIT License](https://opensource.org/license/mit).

## To Future Implementors
Lastly, a word of advice to those who will be undertaking this project in the future: **Don’t just copy and paste the code!**

It is crucial to understand the underlying logic by initially writing the code yourself. Have a thorough understanding of the design and functionality. You might need to debug the program multiple times — use GDB and identify any underlying issues. Finally, carefully review your work to ensure that no detail has been overlooked.
