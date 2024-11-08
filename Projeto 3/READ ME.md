# Communication System Between Clients and Server Using Sockets

This code provides a communication system where clients and a server interact using sockets to perform file system operations.

## Functions

- **Create Command**: `c name type`  
- **Lookup Command**: `l name`
- **Delete Command**: `d name`
- **Print Tree State Command**: `p outputfile`

## Types

- **Directory**: `d`
- **File**: `f`

## Usage

- **Client**: `./tecnicofs-client inputFile nomeserver`
- **Server**: `./tecnicofs numthread nomeserver`
