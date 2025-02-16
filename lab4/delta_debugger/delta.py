import math

from typing import Generator, Tuple

from delta_debugger import run_target

EMPTY_STRING = b""


def delta_debug(target: str, program_input: bytes) -> bytes:
    """
    Delta-Debugging algorithm

    TODO: Implement your algorithm for delta-debugging.

    Hint: It may be helpful to use an auxilary function that
    takes as input a target, input string, n and
    returns the next input and n to try.

    :param target: target program
    :param input: crashing input to be minimized
    :return: 1-minimal crashing input.
    """
    def next_input(program_input: list, n: int) -> Generator[Tuple[bytes, int], None, int]:
        """
        Returns the next input and n to try.

        :param target: target program
        :param input: crashing input to be minimized
        :param n: number of partitions
        :return: next input and n to try.
        """
        # partition input
        partition = math.ceil(len(program_input) / n)
        # yield each partition
        for i in range(0, len(program_input), partition):
            yield program_input[i:i + partition], partition

    def minimize_input(target: str, input_data: list, partition_count: int) -> list:
        """
        Recursively minimize the input data.

        :param target: target program to invoke 
        :param input_data: crashing input to be minimized
        :param partition_count: number of partitions
        :return: minimized input data
        """
        # Check if the empty string causes a crash for early termination
        if run_target(target, EMPTY_STRING) != 0:
            return list(EMPTY_STRING)

        while partition_count <= len(input_data):
            found_crash = False
            partitions = list(next_input(input_data, partition_count))
            for i in range(len(partitions)):
                # Remove the i-th partition
                reduced_input = input_data[:partitions[i][1]] + input_data[partitions[i][1] + len(partitions[i][0]):]
                if run_target(target, bytes(reduced_input)) != 0:
                    # If the reduced input causes a crash, treat it as the current configuration
                    input_data = reduced_input
                    partition_count = 2  # Restart with 2 partitions
                    found_crash = True
                    break

            if not found_crash:
                # If no crashes are found, increase the partition count
                partition_count *= 2
        

        return reduced_input

    # convert data to a list so it can be partitioned
    input_data = list(program_input)
    # start with 2 partitions (basic binary search)
    minimized_input = minimize_input(target, input_data, 2)
    # return back to bytes for caller
    return bytes(minimized_input)
