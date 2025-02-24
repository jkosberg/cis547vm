#! /usr/bin/env python3

from collections import defaultdict
import itertools
from pathlib import Path
from typing import Dict, Iterable, List, Set
from cbi.data_format import (
    CBILog,
    ObservationStatus,
    Predicate,
    PredicateInfo,
    PredicateType,
    Report,
)
import copy
from cbi.utils import get_logs


def collect_observations(log: CBILog) -> Dict[Predicate, ObservationStatus]:
    """
    Traverse the CBILog and collect observation status for each predicate.

    NOTE: If you find a `Predicate(line=3, column=5, pred_type="BranchTrue")`
    in the log, then you have observed it as True,
    further it also means you've observed is complement:
    `Predicate(line=3, column=5, pred_type="BranchFalse")` as False.

    :param log: the log
    :return: a dictionary of predicates and their observation status.
    """
    observations: Dict[Predicate, ObservationStatus] = defaultdict(
        lambda: ObservationStatus.NEVER
    )

    """
    TODO: Add your code here

    Hint: The PredicateType.alternatives will come in handy.
    """
    for valid_predicate in log:
        predicate = Predicate(
            line=valid_predicate.line,
            column=valid_predicate.column,
            value=valid_predicate.value,
        )

        previous_observation = observations.get(predicate, ObservationStatus.NEVER)
        new_observation = ObservationStatus.NEVER

        if valid_predicate.value:
            new_observation = ObservationStatus.ONLY_TRUE
        else:
            new_observation = ObservationStatus.ONLY_FALSE

        if previous_observation != new_observation:
            observations[predicate] = ObservationStatus.merge(
                previous_observation, new_observation
            )
        # We also have to mark all the alternatives as observed
        for type, status in PredicateType.alternatives(valid_predicate.value):
            new_predicate = copy.deepcopy(predicate)
            new_predicate.pred_type = type
            observations[new_predicate] = status
    return observations


def collect_all_predicates(logs: Iterable[CBILog]) -> Set[Predicate]:
    """
    Collect all predicates from the logs.

    :param logs: Collection of CBILogs
    :return: Set of all predicates found across all logs.
    """
    predicates: set[Predicate] = set()

    for input_logs in logs:
        # Each log will have a list of predicates
        for valid_predicate in input_logs:
            # Log will look something like: {"kind": "branch", "line": 5, "column": 7, "value": true}
            predicates.add(
                Predicate(
                    line=valid_predicate.line,
                    column=valid_predicate.column,
                    value=valid_predicate.value,
                )
            )
    return predicates

def find_closest_predicate_info(predicate: Predicate, predicate_infos: Dict[Predicate, PredicateInfo]) -> PredicateInfo:
    """
    Find the closest predicate info for a given predicate.
    This is used to handle the case where the predicate is not found in the
    predicate_infos dictionary.
    :param predicate: the predicate to find
    :param predicate_infos: the predicate infos dictionary
    :return: the closest predicate info
    """
    if predicate not in predicate_infos:
        for p in predicate_infos.keys():
            if p.line == predicate.line and p.column == predicate.column:
                return predicate_infos[p]
    return predicate_infos[predicate]

def cbi(success_logs: List[CBILog], failure_logs: List[CBILog]) -> Report:
    """
    Compute the CBI report.

    :param success_logs: logs of successful runs
    :param failure_logs: logs of failing runs
    :return: the report
    """
    all_predicates = collect_all_predicates(itertools.chain(success_logs, failure_logs))

    predicate_infos: Dict[Predicate, PredicateInfo] = {
        pred: PredicateInfo(pred) for pred in all_predicates
    }

    # TODO: Add your code here to compute the information for each predicate.

        # Get all successes and failures and update them
    for log in success_logs:
        # Populate the base information for each predicate
        for pred in log:
            predicate = Predicate(line=pred.line, column=pred.column, value=pred.value)
            predicate_info: PredicateInfo = predicate_infos[predicate]
            predicate_info.num_true_in_success += 1
        
        # Now we can populate information from the observations
        observations = collect_observations(log)
        for predicate, observation in observations.items():
            predicate_info = predicate_infos.get(predicate, PredicateInfo(predicate))
            # now we increment number observed in failure/success for each predicate
            if observation == ObservationStatus.ONLY_TRUE:
                predicate_info.num_observed_in_success += 1
            if observation == ObservationStatus.ONLY_FALSE:
                predicate_info.num_observed_in_success += 1
            if observation == ObservationStatus.BOTH:
                predicate_info.num_observed_in_success += 1
                predicate_info.num_observed_in_failure += 1
            predicate_infos[predicate] = predicate_info
            

                            
    for log in failure_logs:
        # Populate the base information for each predicate
        for pred in log:
            predicate = Predicate(line=pred.line, column=pred.column, value=pred.value)
            predicate_info = predicate_infos[predicate]
            predicate_info.num_true_in_failure += 1
        
        # Now we can populate information from the observations
        observations = collect_observations(log)
        for predicate, observation in observations.items():
            predicate_info = predicate_infos.get(predicate, PredicateInfo(predicate))
            # now we increment number observed in failure/success for each predicate
            if observation == ObservationStatus.ONLY_TRUE:
                predicate_info.num_observed_in_failure += 1
            if observation == ObservationStatus.ONLY_FALSE:
                predicate_info.num_observed_in_failure += 1
            if observation == ObservationStatus.BOTH:
                predicate_info.num_observed_in_failure += 1
                predicate_info.num_observed_in_success += 1
            predicate_infos[predicate] = predicate_info

    # Finally, create a report and return it.
    report = Report(predicate_info_list=list(predicate_infos.values()))
    return report