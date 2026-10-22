# TWI State-Machine

## Features

- Performs autonomous communications after configuration and boot
- Modifiable order of the operations
- Manages transmission of data in both directions
- Uses specific buffers with indefinite length to load/store data
- Handles TWI-related errors and hardware timeout
- Implements a non-blocking ISR to advance to the next state
- Allows communication frequency configuration
- Guarantees hardware liberation before returning

## Data Structure Overview

```c
typedef struct {
	twi_operations next_op;
	void (*next_op_ptr)(void);
	uint16_t input[2];
} twi_state_branch;

typedef struct {
	twi_operations current_op;
	twi_state_branch branches[TWI_STATES_NUMBER];
} twi_state;

typedef 	void (*function_ptr)(void);

typedef struct {
	volatile uint8_t status_code;
	volatile twi_errors error;
	twi_state states[TWI_STATES_NUMBER];
	volatile twi_state current_state;
	volatile function_ptr next_op_ptr;
	volatile uint16_t current_input;
	uint8_t sla_add;
	volatile const uint8_t* tx_buff;
	volatile uint8_t* rx_buff;
	volatile uint8_t stop_sent;
} twi_controller;

static twi_controller twi_state_machine = {0};
```

---

## Mealy State-Machine

The implemented state-machine is a Mealy finite state-machine, where each state is distinguished by the last performed operation,
and the input is the progressive index of the current state.

Using the progressive index of the state as input allows the machine to operate autonomously: the caller is only required to configure and boot it,
then each time the machine advances to the next state the progressive index is automatically increased, making the machine produce its own input
until the final state.

The design integrates nested structures to obtain configuration parameters, state values and 7 states (every I2C operation).
Each state has 7 branches pointing to every state, so that every branch points to a different state and has a configurable input range.

At every state the machine advances to the next one by searching the branch with an input range containing the current value of the progressive index.

For example: the DATA SENT state has a branch pointing to itself with a range [3, 6], then has a branch pointing to the STOP state
with a range [7, 7]. In this way, once the machine is in the DATA SENT state and the current state index is 3, it will move to
the DATA SENT state again for 4 times (performing 4 DATA SEND operations) and finally to the STOP state.

## ISR

The ISR is only responsible to check for possible errors caused by the previous operation, call `twi_state_advance` and perform the next TWI
operation by calling the corresponding function through `next_op_ptr`:

```c
switch (tsm.current_state.current_op) {
	
	case START:
			
		if (TW_STATUS != TW_START) {
			tsm.status_code = TW_STATUS;
			tsm.error = TWI_ERR_START;
			twi_stop_send();
			break;
		}
			
		twi_state_advance();
		break;
```

then:

```c
if (tsm.error == TWI_ERR_OK && tsm.next_op_ptr != NULL) {
	tsm.next_op_ptr();
}
```

`twi_state_advance` has a `for` loop with a finite number of steps, which makes the ISR duration deterministic.

## State Advancement Logic

The advancement is completely handled by `twi_state_advance`, which follows the flow:

```text
1. Use a `for` loop to search for the branch of the current state which has an input range containing the current index value
2. Copy from that branch the pointer to the function that runs the next operation
3. Changes the configuration parameter indicating the current state so that it matches the next operation
4. Increments the progressive index value
5. Steps (3) and (4) are skipped if the branch points to the STOP state
```

To ensure the correct execution of `twi_state_advance` 2 preconditions are required:

```text
1. Every used state has at least one branch pointing to the next state within an input range
2. All input ranges of a used states are non-intersecting intervals with each others
```

In fact, precondition (2) is a limitation: the correct execution is not guaranteed if the state-machine is configured to make an advancement
from state A to state B, then to make a future advancement from A to C and then to make another future advancement again from A to B.

The reason is that if the branch pointing to B has range [2, 6], the branch pointing to C has range [3, 4], and the branch pointing to B is examined
earlier in the `for` loop, then once the progressive index is set to 3 and 4 the `for` loop will always stop at the branch one pointing to B even
before checking the other branch input range.

## Boot and Finish Logic

When a module calls `twi_master_transmitter` or `twi_master_receiver` the following flow is executed:

```text
1. The input range of every branch of all the states is set to [0, 0] to guarantee that unused states will not be selected by the `for` loop
2. At least one branch in every used state is configured to have an input range starting at least from 1
3. `twi_state_machine_boot` is called to set the communication frequency, save the tx/rx buffer by coping the passed pointer and set the current state to START
4. The hardware timeout is employed to ensure that the STOP condition is sent and the TWI hardware is liberated within 30 ms
```

---

## Blocking API

The only firmware blocking API is in `twi` where the hardware timeout is employed. This decision is justified because the longest I2C transactions take
up to ~15 ms, and this has never caused issues during the tests.

## Portability

The designed structure can be reused as a Mealy state‑machine with each branch having `enum` constants as inputs, which can be labeled to represent
a user interaction,an operation or a state of another state‑machine.
