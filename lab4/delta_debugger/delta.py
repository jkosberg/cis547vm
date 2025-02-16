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

    def delta_debugging(input_data: list, test_function: callable) -> list:
        """
        Implements the delta debugging algorithm to minimize the input data that causes a failure.

        :param input_data: The input data to be minimized.
        :param test_function: A function that returns True if the input data causes a failure, False otherwise.
        :return: The minimized input data that still causes the failure.
        """
        # early return with base case if empty string causes failure (aka program just always fails)
        if test_function(EMPTY_STRING):
            return EMPTY_STRING
        
        def dd_helper(data: list, n: int) -> list:
            """
            Helper function for delta debugging.
            :param data: The input data to be minimized.
            :param n: The number of partitions to divide the data into.
            :return: The minimized input data that still causes the failure.
            """
            # base case where we can't partition further
            if n > len(data):
                return data
            
            # partition data into n chunks
            chunk_size = len(data) // n
            for i in range(n):
                # Create a chunk of the original data
                chunk = data[i * chunk_size:(i + 1) * chunk_size]
                # Create the rest of the data without the current chunk
                rest = data[:i * chunk_size] + data[(i + 1) * chunk_size:]

                # If the rest of the data still causes a failure, recurse with the rest
                if test_function(rest):
                    return dd_helper(rest, 2)
                # If the current chunk causes a failure, recurse with the chunk
                if test_function(chunk):
                    return dd_helper(chunk, 2)

            # If no smaller input causes a failure, recurse with the original data
            if n < len(data):
                return dd_helper(data, min(len(data), 2 * n))
            return data

        # Start the delta debugging process with the initial input data and partition size of 2
        return dd_helper(input_data, 2)
    
    def test_function(input_data: list) -> bool: 
        """Function to test if the input data causes a failure."""
        return run_target(target, bytes(input_data)) != 0

    # convert data to a list so it can be partitioned
    input_data = list(program_input)
    minimized_input = delta_debugging(input_data, test_function)
    # return back to bytes for caller
    return bytes(minimized_input)
