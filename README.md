# i3ds-wisdom

I3DS interface for the WISODM GPR. The project creates two executables: 

* **i3ds\_wisdom** which creates a sensor node that accepts I3DS commands and sends UDP messages to the WISDOM server.
* **wisdom\_ack\_service** which listenes for UDP messages and sends an ACK after a given time interval.

## Building

First, install **i3ds-framework-cpp** and all its dependencies. Then, from the project root folder:

```bash
mkdir build
cd build
cmake ..
make
```

The executables can then be installed with

```bash
sudo make install
```

or just run from the build folder.

## Testing
There are two ways to test the **i3ds_wisdom** program without an actual WISDOM GPR: 

1. Running **i3ds\_wisdom** in dummy mode
2. Running **i3ds\_wisdom** together with **wisdom\_ack\_service** 

### Running **i3ds\_wisdom** in dummy mode
When **ì3ds\_wisdom** is run in dummy mode, it will not send any UDP packets. Instead it will print messages to the console and automatically change its own state when it receives a command. When a *start* command is sent, it will go to the *operational* state and wait for a configured number of seconds before it returns to the *standby* state. To run **i3ds\_wisdom** in dummy mode, run it with the `-d` flag and a number greater than 0, which will be the duration it will wait while "taking a measurement". To run the service with node number 25 and a measurement delay of 5 seconds:

```bash
i3ds_wisdom -n 25 -d 5
```

We can then activate and start the sensor from another terminal with the commands

```bash
i3ds_configure_sensor -n 25 --activate
i3ds_configure_sensor -n 25 --start
```

Then we can see the **i3ds\_wisdom** instance print its status messages. We can also query the state of the sensor from the other terminal with

```bash
i3ds_configure_sensor -n 25 --print
```

This will show that the WISDOM sensor transitions between the *standby* and *operational* states.

### Running i3ds\_wisdom together with wisdom\_ack\_service
This setup runs the **i3ds\_wisdom** node as if it was communicating with the real WISDOM server, and uses the **wisdom\_ack\_service** to listen for UDP messages and send ACK messages back after a configured delay.

> ***WARNING*** The **wisdom\_ack\_service** is only made to test the UDP functionality. It does not aim to be an accurate representation of how the actual WISDOM server works.

Run the **wisdom\_ack\_service** with a chosen port with

```bash
wisdom_ack_service -p 12345
```

and in another terminal, run the **i3ds\_wisdom** service with

```bash
i3ds_wisdom -n 25 -p 12345
```

This time, we run it with the `-p` flag and a port number instead of the `-d` flag. It will now send actual UDP messages to the chosen port number on the local machine and wait for an ACK message. *Note: It does not currently check what the ACK message contains, just that it receives one.* Then we can send commands using **i3ds\_configure\_sensor** like before.

## Run with a real GPR
To run the system with a real WISDOM GPR, just run the **i3ds\_wisdom** service:

```bash
i3ds_wisdom -n <node> -p <port>
```

using a chosen node ID and the port of the WISDOM server. If the server is running on another machine, use

```bash
i3ds_wisdom -n <node> -p <port> -i <ip-address>
```
