# This Python file uses the following encoding: utf-8
############################################################################
# Copyright (c) 2025 Li Auto Inc. and its affiliates
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################
"""
Ftrace Log Parser Module

This module parses ftrace log files to extract task timing information:
- Task names and priorities
- Task execution times (max/avg/std_dev)
- Task periods
- Other task metrics
"""

import json
import math
import os
import re
import sys
import traceback
from pathlib import Path
from typing import Dict, Any, Optional, List, Tuple
from datetime import datetime
import tkinter as tk
from tkinter import messagebox, simpledialog

# Import matplotlib for plotting
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MaxNLocator

# Add ftrace parser to path
base_directory = os.path.dirname(os.path.abspath(__file__))
ftrace_parser_directory = os.path.abspath(os.path.join(
    base_directory, '..', '..', '..', '..', 'vcos_studio', 'halo_trace'))
sys.path.append(ftrace_parser_directory)
from ftrace_manager import FtraceParser

# Available project configurations
PROJECT_CONFIGS = {
    "TC397": "tc397_cfg",
    "E3650": "e3650_cfg"
}

class FtraceLogParser:
    """
    Parser for ftrace log files containing task timing information.

    Extracts task execution metrics including:
    - Task names and priorities
    - Execution times (max/avg/std_dev)
    - Task periods
    """
    def __init__(self, ftrace_file: str):
        self.ftrace_file = ftrace_file
        self.input_task_dict = {}
        self.core_pattern = re.compile(r'.*Core\d+.*')
        self.priority_cache = {}

    def collect_task_names(self) -> None:
        """Collect all task names from ftrace file that contain core identifiers"""
        try:
            with open(self.ftrace_file, 'r', encoding='utf-8') as f:
                for line in f:
                    if 'sched_switch' in line:
                        for match in re.finditer(r'(?:prev|next)_comm=([^\s]+)', line):
                            task_name = match.group(1)
                            if self.core_pattern.match(task_name):
                                self.input_task_dict[task_name] = [None, None]
                                if task_name not in self.priority_cache:
                                    if f'prev_comm={task_name}' in line:
                                        prio_match = re.search(r'prev_prio=(\d+)', line)
                                        if prio_match:
                                            self.priority_cache[task_name] = int(
                                                prio_match.group(1))
                                    elif f'next_comm={task_name}' in line:
                                        prio_match = re.search(r'next_prio=(\d+)', line)
                                        if prio_match:
                                            self.priority_cache[task_name] = int(
                                                prio_match.group(1))
        except Exception as e:
            print(f"Error collecting task names: {str(e)}")
            raise

    def get_task_priority(self, task_name: str) -> int:
        """
        Get task priority from cache or ftrace file

        Args:
            task_name: Name of the task to find priority for

        Returns:
            Task priority as integer or -1 if not found
        """
        if task_name in self.priority_cache:
            return self.priority_cache[task_name]

        try:
            with open(self.ftrace_file, 'r', encoding='utf-8') as f:
                for line in f:
                    if task_name in line and 'sched_switch' in line:
                        for prefix, prio_prefix in [
                            ('prev_comm=', 'prev_prio='),
                            ('next_comm=', 'next_prio=')
                        ]:
                            if f'{prefix}{task_name}' in line:
                                prio_match = re.search(f'{prio_prefix}(\\d+)', line)
                                if prio_match:
                                    priority = int(prio_match.group(1))
                                    self.priority_cache[task_name] = priority
                                    return priority
        except Exception as e:
            print(f"Error getting priority for task {task_name}: {str(e)}")

        return -1

    def get_task_parameters(self) -> Dict[str, Dict[str, Any]]:
        """
        Extract task parameters from ftrace file

        Returns:
            Dictionary mapping task names to their parameters
        """
        try:
            self.collect_task_names()

            if not self.input_task_dict:
                print("Warning: No valid tasks found in ftrace file")
                return {}

            success, task_info = FtraceParser.get_ftrace_task_info(
                self.ftrace_file, self.input_task_dict)

            if not success or not task_info:
                print("Failed to parse ftrace file or no task info found")
                return {}

            task_params = {}
            for task_name, params in task_info.items():
                try:
                    if not self.core_pattern.search(task_name):
                        continue

                    priority = self.get_task_priority(task_name)
                    period = round(params.get("dt_avg", 0) * 1e6) if params.get("dt_avg", 0) > 0 else 0
                    cet_max = round(params.get("cet_max", 0) * 1e6)
                    cet_avg = round(params.get("cet_avg", 0) * 1e6)
                    cet_std_dev = round(params.get("cet_std_dev", 0) * 1e6)

                    # Get PCET list and convert units to microseconds
                    pcet = params.get("PCET", [])
                    if pcet:
                        pcet = [round(x) for x in pcet]  # Convert to microseconds and round

                    # Get DT list (deadline time samples)
                    dt_list = params.get("dt_list", [])
                    if dt_list:
                        dt_list = [round(x * 1e6) for x in dt_list]  # Convert to microseconds and round

                    # Get CET list (execution time samples)
                    cet_list = params.get("cet_list", [])
                    if cet_list:
                        cet_list = [round(x * 1e6) for x in cet_list]  # Convert to microseconds and round

                    if period > 0 and (cet_max > 0 or cet_avg > 0):
                        task_params[task_name] = {
                            'priority': priority,
                            'period': period,
                            'cet_max': cet_max,
                            'cet_avg': cet_avg,
                            'cet_std_dev': cet_std_dev,
                            'PCET': pcet,
                            'dt_list': dt_list,
                            'cet_list': cet_list
                        }
                except Exception as e:
                    print(f"Error processing task {task_name}")
                    continue

            return task_params

        except Exception as e:
            print(f"Error in get_task_parameters: {str(e)}")
            raise


def parse_ftrace_log(ftrace_file: str) -> Optional[Dict[str, Dict[str, Any]]]:
    """
    Parse ftrace log file and extract task information

    Args:
        ftrace_file: Path to ftrace log file

    Returns:
        Dictionary of task parameters or None if parsing fails
    """
    try:
        # Normalize file path
        ftrace_path = Path(ftrace_file).resolve()
        if not ftrace_path.exists():
            print(f"Error: Ftrace file not found: {ftrace_path}")
            return None
        if not ftrace_path.is_file():
            print(f"Error: Path is not a file: {ftrace_path}")
            return None

        # Check if file is readable
        if not os.access(ftrace_path, os.R_OK):
            print(f"Error: No read permission for file: {ftrace_path}")
            return None

        parser = FtraceLogParser(str(ftrace_path))
        task_params = parser.get_task_parameters()

        # Validate returned data structure
        if not isinstance(task_params, dict):
            print(f"Error: Invalid task parameters format. "
                  f"Expected dict, got {type(task_params)}")
            return None
        if not task_params:
            print("Warning: No valid task parameters found in ftrace file")
            return None

        # Validate data content
        for task_name, params in task_params.items():
            if not isinstance(params, dict):
                print(f"Error: Invalid parameters format for task {task_name}")
                return None
            required_keys = {'priority', 'period', 'cet_max', 'cet_avg', 'cet_std_dev', 'PCET'}
            if not all(key in params for key in required_keys):
                print(f"Error: Missing required parameters for task {task_name}")
                return None

        return task_params

    except FileNotFoundError as e:
        print(f"File not found error: {str(e)}")
        return None
    except PermissionError as e:
        print(f"Permission error: {str(e)}")
        return None
    except json.JSONDecodeError as e:
        print(f"JSON parsing error: {str(e)}")
        return None
    except Exception as e:
        print(f"Unexpected error while parsing ftrace log: {str(e)}")
        print(f"Error type: {type(e).__name__}")
        traceback.print_exc()
        return None


def print_formatted_output(task_params: Dict[str, Dict[str, Any]], plot_type: str = None) -> None:
    """
    Print formatted task information with improved layout

    Args:
        task_params: Dictionary of task parameters
        plot_type: Type of samples to display (dt or cet)
    """
    # Calculate the maximum length needed for PCET values
    max_pcet_length = 0
    for params in task_params.values():
        pcet_str = f"[{', '.join(str(int(x)) for x in params['PCET'])}]"
        max_pcet_length = max(max_pcet_length, len(pcet_str))

    print("\nTask Parameters By Core:")
    print("=" * 150)  # Extend the separator line

    # Format string for consistent column widths
    header_format = (
        f"{'Task Name':<40} {'Priority':<10} {'Period(us)':<12} {'CET Max(us)':<12} "
        f"{'CET Avg(us)':<12} {'CET StdDev(us)':<12} {'PCET(us)':<{max_pcet_length}} | {'Samples':<8}"
    )

    # Group tasks by core
    cores_dict = {}
    for task_name, params in task_params.items():
        core_match = re.search(r'Core(\d+)', task_name)
        if core_match:
            core_name = f"Core{core_match.group()}"
            if core_name not in cores_dict:
                cores_dict[core_name] = []
            cores_dict[core_name].append((task_name, params))

    # Print tasks by core
    for core_name, tasks in sorted(cores_dict.items()):
        print(f"\n{core_name}:")
        print("-" * 150)  # Extend the separator line
        print(header_format)
        print("-" * 150)  # Extend the separator line

        for task_name, params in sorted(tasks, key=lambda x: x[1]['priority'], reverse=True):
            # Format PCET list
            pcet_str = f"[{', '.join(str(int(x)) for x in params['PCET'])}]"
            pcet_str = f"{pcet_str:<{max_pcet_length}}"  # Fixed PCET column width

            # Get sample count based on plot_type
            # Only show DT samples if plot_type is explicitly 'dt'
            # Otherwise default to CET samples
            if plot_type == 'dt':
                samples_count = len(params.get('dt_list', []))
            else:  # Default to CET samples for no argument or cet argument
                samples_count = len(params.get('cet_list', []))

            samples_str = f"{samples_count:>8}"  # Right-align sample count without "samples" text

            # Print task information with aligned columns
            print(
                f"{task_name:<40} {params['priority']:<10} {params['period']:<12} "
                f"{params['cet_max']:<12} {params['cet_avg']:<12} {params['cet_std_dev']:<12} "
                f"{pcet_str} | {samples_str}"
            )

        print("-" * 150)  # Extend the separator line


def organize_tasks_by_core(
    task_params: Dict[str, Dict[str, Any]]
) -> Dict[str, Dict[str, Dict[str, Any]]]:
    """
    Organize task parameters by core and sort tasks by priority (high to low)

    Args:
        task_params: Raw task parameters dictionary

    Returns:
        Dictionary organized by core with tasks sorted by priority:
        {
            "Core0": {
                "Task1": {
                    "priority": int,
                    "period": int,
                    "cet_max": int,
                    "cet_avg": int,
                    "cet_std_dev": int,
                    "dt_list": [...]
                },
                ...
            },
            "Core1": {
                ...
            },
            ...
        }
    """
    cores_dict = {}
    # Regular expression to match core identifiers like "Core0", "Core1", etc.
    core_pattern = re.compile(r"Core\d+")

    # First, organize tasks by core
    for task_name, params in task_params.items():
        # Search for core identifier in the task name
        core_match = core_pattern.search(task_name)
        if core_match:
            core_name = core_match.group()
        else:
            # If no core identifier is found, skip the task
            print(f"Warning: No core identifier found in task name '{task_name}'. "
                  f"Skipping task.")
            continue

        if core_name not in cores_dict:
            cores_dict[core_name] = {}
        cores_dict[core_name][task_name] = params

    # Then, sort tasks in each core by priority (high to low)
    sorted_cores_dict = {}
    for core_name, core_tasks in cores_dict.items():
        # Sort tasks by priority (high to low)
        sorted_tasks = dict(sorted(
            core_tasks.items(),
            key=lambda x: x[1]['priority'],
            reverse=True  # High to low
        ))
        sorted_cores_dict[core_name] = sorted_tasks

    return sorted_cores_dict


def save_cores_dict_to_json(
    cores_dict: Dict[str, Dict[str, Dict[str, Any]]],
    file_path: str
) -> None:
    """
    Save cores dictionary to a JSON file

    Args:
        cores_dict: Dictionary containing core-wise task parameters
        file_path: Path to save the JSON file
    """
    try:
        # Ensure directory exists
        output_dir = Path(file_path).parent
        output_dir.mkdir(parents=True, exist_ok=True)

        # Write to JSON file with pretty formatting
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(cores_dict, f, indent=4, sort_keys=False)
        print(f"\nSuccessfully saved cores dictionary to: {file_path}")
    except Exception as e:
        print(f"Error saving JSON file: {str(e)}")


def plot_dt_samples(
    task_name: str,
    dt_list: List[float],
    output_dir: str,
    period: int = 0,
    fixed_y_range: bool = True
) -> None:
    """
    Plot deadline time samples for a task and save the graph

    Args:
        task_name: Name of the task
        dt_list: List of deadline time samples (in microseconds)
        output_dir: Directory to save the plot image
        period: Task period in microseconds (if available)
        fixed_y_range: If True, use a fixed Y-axis range based on period
    """
    if not dt_list:
        print(f"Warning: No DT samples available for task '{task_name}'")
        return

    try:
        # Create directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Get statistics for better visualization
        min_dt = min(dt_list)
        max_dt = max(dt_list)
        mean_dt = np.mean(dt_list)
        std_dt = np.std(dt_list)

        # Create a figure with a specific layout to control legend placement
        fig, ax = plt.subplots(figsize=(12, 8), dpi=120)

        # Create x-axis (sample index)
        x = np.arange(len(dt_list))

        # Plot the main data
        ax.plot(x, dt_list, color='#1f77b4', linewidth=0.8, alpha=0.7)

        # Don't plot points for large datasets to reduce visual clutter
        if len(dt_list) < 1000:
            ax.scatter(x, dt_list, color='#ff7f0e', s=10, alpha=0.7)

        # Add reference lines
        ax.axhline(y=mean_dt, color='r', linestyle='-', linewidth=1.5,
                  label=f'Mean: {mean_dt:.2f} μs')
        ax.axhline(y=mean_dt + std_dt, color='g', linestyle='--', linewidth=1,
                  label=f'Mean + StdDev: {mean_dt + std_dt:.2f} μs')
        ax.axhline(y=mean_dt - std_dt, color='g', linestyle='--', linewidth=1,
                  label=f'Mean - StdDev: {mean_dt - std_dt:.2f} μs')

        # Add period reference line if provided
        if period > 0:
            ax.axhline(y=period, color='purple', linestyle='-.', linewidth=1.5,
                      label=f'Period: {period} μs')

        # Add grid with better styling
        ax.grid(True, linestyle='--', alpha=0.3)

        # Set labels and title with better font
        ax.set_xlabel('Sample Index', fontsize=12, fontweight='bold')
        ax.set_ylabel('Deadline Time (μs)', fontsize=12, fontweight='bold')
        ax.set_title(f'Deadline Time Samples for {task_name}', fontsize=14, fontweight='bold')

        # Create legend in the upper left
        legend1 = ax.legend(
            loc='upper left',
            frameon=True,
            fancybox=True,
            shadow=True
        )

        # Set y-axis range with consistent scale for same-period tasks
        if fixed_y_range and period > 0:
            # Calculate a fixed range based on the period
            # For 1ms period tasks, use range from 700 to 1400
            if 900 <= period <= 1100:  # Around 1ms period
                y_min = 700
                y_max = 1400
            # For 5ms period tasks
            elif 4900 <= period <= 5100:  # Around 5ms period
                y_min = 4500
                y_max = 5500
            # For 10ms period tasks
            elif 9900 <= period <= 10100:  # Around 10ms period
                y_min = 9000
                y_max = 11000
            # For other periods, calculate a reasonable range
            else:
                y_min = max(0, period - period * 0.3)
                y_max = period + period * 0.4

            ax.set_ylim(y_min, y_max)
        else:
            # Use data-driven range with margin if fixed range is not requested
            margin = (max_dt - min_dt) * 0.1 if max_dt > min_dt else mean_dt * 0.1
            ax.set_ylim(max(0, min_dt - margin), max_dt + margin)

        # For large datasets, use fewer x-axis labels to avoid crowding
        if len(dt_list) > 1000:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins=10))
        else:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        # Create statistics text box
        stats_text = (
            f"Statistics:\n"
            f"Samples: {len(dt_list)}\n"
            f"Min: {min_dt:.2f} μs\n"
            f"Max: {max_dt:.2f} μs\n"
            f"Mean: {mean_dt:.2f} μs\n"
            f"StdDev: {std_dt:.2f} μs"
        )

        # Add annotation box in the upper right corner
        ax.annotate(
            stats_text,
            xy=(0.99, 0.99),
            xycoords='axes fraction',
            fontsize=10,
            bbox=dict(boxstyle="round,pad=0.5", facecolor='white', alpha=0.8),
            ha='right',
            va='top'
        )

        # Adjust layout to make room for all elements
        fig.tight_layout()

        # Save the figure with higher quality
        safe_task_name = re.sub(r'[^\w\-_.]', '_', task_name)  # Replace invalid characters
        file_path = os.path.join(output_dir, f"{safe_task_name}_DT_Samples.png")
        fig.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close(fig)

        print(f"Created DT samples plot for '{task_name}' at: {file_path}")

    except Exception as e:
        print(f"Error creating plot for task '{task_name}': {str(e)}")
        traceback.print_exc()


def plot_all_task_dt_samples(
    task_params: Dict[str, Dict[str, Any]],
    output_dir: str
) -> None:
    """
    Plot deadline time samples for all tasks and save the graphs

    Args:
        task_params: Dictionary of task parameters
        output_dir: Directory to save the plot images
    """
    print("\nGenerating DT sample plots for all tasks...")

    # Track the number of plots created
    plot_count = 0

    for task_name, params in task_params.items():
        dt_list = params.get('dt_list', [])
        period = params.get('period', 0)
        if dt_list:
            plot_dt_samples(task_name, dt_list, output_dir, period)
            plot_count += 1

    if plot_count > 0:
        print(f"\nSuccessfully created {plot_count} DT sample plots")
    else:
        print("\nNo DT sample plots were created. No tasks have DT samples.")


def plot_cet_samples(
    task_name: str,
    cet_list: List[float],
    output_dir: str,
    fixed_y_range: bool = True
) -> None:
    """
    Plot execution time samples for a task and save the graph

    Args:
        task_name: Name of the task
        cet_list: List of execution time samples (in microseconds)
        output_dir: Directory to save the plot image
        fixed_y_range: If True, use a fixed Y-axis range based on data
    """
    if not cet_list:
        print(f"Warning: No CET samples available for task '{task_name}'")
        return

    try:
        # Create directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)

        # Get statistics for better visualization
        min_cet = min(cet_list)
        max_cet = max(cet_list)
        mean_cet = np.mean(cet_list)
        std_cet = np.std(cet_list)

        # Create a figure with a specific layout to control legend placement
        fig, ax = plt.subplots(figsize=(12, 8), dpi=120)

        # Create x-axis (sample index)
        x = np.arange(len(cet_list))

        # Plot the main data
        ax.plot(x, cet_list, color='#1f77b4', linewidth=0.8, alpha=0.7)

        # Don't plot points for large datasets to reduce visual clutter
        if len(cet_list) < 1000:
            ax.scatter(x, cet_list, color='#ff7f0e', s=10, alpha=0.7)

        # Add reference lines
        ax.axhline(y=mean_cet, color='r', linestyle='-', linewidth=1.5,
                  label=f'Mean: {mean_cet:.2f} μs')
        ax.axhline(y=mean_cet + std_cet, color='g', linestyle='--', linewidth=1,
                  label=f'Mean + StdDev: {mean_cet + std_cet:.2f} μs')
        ax.axhline(y=mean_cet - std_cet, color='g', linestyle='--', linewidth=1,
                  label=f'Mean - StdDev: {mean_cet - std_cet:.2f} μs')

        # Add grid with better styling
        ax.grid(True, linestyle='--', alpha=0.3)

        # Set labels and title with better font
        ax.set_xlabel('Sample Index', fontsize=12, fontweight='bold')
        ax.set_ylabel('Execution Time (μs)', fontsize=12, fontweight='bold')
        ax.set_title(f'Execution Time Samples for {task_name}', fontsize=14, fontweight='bold')

        # Create legend in the upper left
        legend1 = ax.legend(
            loc='upper left',
            frameon=True,
            fancybox=True,
            shadow=True
        )

        # Set y-axis range with consistent scale
        if fixed_y_range:
            # Calculate a reasonable range based on data
            y_min = max(0, min_cet - std_cet * 0.5)
            y_max = max_cet + std_cet * 0.5
            ax.set_ylim(y_min, y_max)
        else:
            # Use data-driven range with margin if fixed range is not requested
            margin = (max_cet - min_cet) * 0.1 if max_cet > min_cet else mean_cet * 0.1
            ax.set_ylim(max(0, min_cet - margin), max_cet + margin)

        # For large datasets, use fewer x-axis labels to avoid crowding
        if len(cet_list) > 1000:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True, nbins=10))
        else:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))

        # Create statistics text box
        stats_text = (
            f"Statistics:\n"
            f"Samples: {len(cet_list)}\n"
            f"Min: {min_cet:.2f} μs\n"
            f"Max: {max_cet:.2f} μs\n"
            f"Mean: {mean_cet:.2f} μs\n"
            f"StdDev: {std_cet:.2f} μs"
        )

        # Add annotation box in the upper right corner
        ax.annotate(
            stats_text,
            xy=(0.99, 0.99),
            xycoords='axes fraction',
            fontsize=10,
            bbox=dict(boxstyle="round,pad=0.5", facecolor='white', alpha=0.8),
            ha='right',
            va='top'
        )

        # Adjust layout to make room for all elements
        fig.tight_layout()

        # Save the figure with higher quality
        safe_task_name = re.sub(r'[^\w\-_.]', '_', task_name)  # Replace invalid characters
        file_path = os.path.join(output_dir, f"{safe_task_name}_CET_Samples.png")
        fig.savefig(file_path, dpi=300, bbox_inches='tight')
        plt.close(fig)

        print(f"Created CET samples plot for '{task_name}' at: {file_path}")

    except Exception as e:
        print(f"Error creating CET plot for task '{task_name}': {str(e)}")
        traceback.print_exc()


def plot_all_task_cet_samples(
    task_params: Dict[str, Dict[str, Any]],
    output_dir: str
) -> None:
    """
    Plot execution time samples for all tasks and save the graphs

    Args:
        task_params: Dictionary of task parameters
        output_dir: Directory to save the plot images
    """
    print("\nGenerating CET sample plots for all tasks...")

    # Track the number of plots created
    plot_count = 0

    for task_name, params in task_params.items():
        cet_list = params.get('cet_list', [])
        if cet_list:
            plot_cet_samples(task_name, cet_list, output_dir)
            plot_count += 1

    if plot_count > 0:
        print(f"\nSuccessfully created {plot_count} CET sample plots")
    else:
        print("\nNo CET sample plots were created. No tasks have CET samples.")


def select_project_config():
    """
    Display a GUI dialog for the user to select a project configuration

    Returns:
        str: Selected project configuration name
    """
    root = tk.Tk()
    root.withdraw()  # Hide the main window

    # Create a custom dialog for project selection
    class ProjectSelectionDialog(simpledialog.Dialog):
        def __init__(self, parent):
            self.selection = None
            super().__init__(parent, "Project Configuration Path Selection")

        def body(self, frame):
            # Set minimum width for the dialog
            self.minsize(width=400, height=150)

            # Title label with larger font and bold
            title_font = ('Arial', 11, 'bold')
            title_label = tk.Label(frame, text="Select a project path configuration:", font=title_font)
            title_label.grid(row=0, columnspan=2, pady=15, padx=20, sticky="w")

            # Create variable for radio button selection
            self.var = tk.StringVar(value="TC397")  # Default selection

            # Create radio buttons for each project config
            radio_font = ('Arial', 10)
            for i, config_name in enumerate(PROJECT_CONFIGS.keys()):
                tk.Radiobutton(frame, text=config_name, variable=self.var, 
                              value=config_name, font=radio_font).grid(row=i+1, column=0, sticky="w", padx=40, pady=5)

            return frame

        def buttonbox(self):
            # Override to create custom buttons with better size
            box = tk.Frame(self)

            ok_button = tk.Button(box, text="OK", width=10, command=self.ok, default=tk.ACTIVE)
            ok_button.pack(side=tk.LEFT, padx=20, pady=10)
            cancel_button = tk.Button(box, text="Cancel", width=10, command=self.cancel)
            cancel_button.pack(side=tk.LEFT, padx=20, pady=10)

            self.bind("<Return>", self.ok)
            self.bind("<Escape>", self.cancel)

            box.pack()

        def apply(self):
            self.selection = self.var.get()

    # Display dialog and get selection
    dialog = ProjectSelectionDialog(root)

    # If user closed dialog without selecting, default to TC397
    selected_config = dialog.selection if dialog.selection else "TC397"

    # Clean up
    root.destroy()

    return selected_config

def check_file_exists(file_path):
    """Check if a file exists and return result"""
    return os.path.isfile(file_path)

def get_project_paths(project_config):
    """
    Get the file paths for the selected project configuration

    Args:
        project_config: Selected project configuration name

    Returns:
        tuple: (ftrace_file_path, json_output_path, dt_plots_dir, cet_plots_dir)
    """
    # Get the configuration folder name
    config_folder = PROJECT_CONFIGS.get(project_config, "tc397_cfg")  # Default to tc397_cfg

    # Add timestamp to output directory names for better organization
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    # Build the paths based on project configuration
    base_path = f".//..//..//..//..//examples//rt_demo//platform_cfg//{config_folder}"

    ftrace_file = f"{base_path}//config//Import//{project_config}_ftrace.log"
    json_output = f"{base_path}//config//Export//ftrace_task_info_{timestamp}.json"
    dt_plots_dir = f"{base_path}//config//Export//DT_Sample_Plots_{timestamp}"
    cet_plots_dir = f"{base_path}//config//Export//CET_Sample_Plots_{timestamp}"

    return ftrace_file, json_output, dt_plots_dir, cet_plots_dir

if __name__ == '__main__':
    try:
        # Get command line arguments
        plot_type = None

        if len(sys.argv) > 1:
            if sys.argv[1].lower() == 'dt':
                plot_type = 'dt'
            elif sys.argv[1].lower() == 'cet':
                plot_type = 'cet'

        # Let user select which project configuration to use
        selected_project = select_project_config()

        # Get the paths for the selected project
        FTRACE_FILE, JSON_OUTPUT, DT_PLOTS_DIR, CET_PLOTS_DIR = get_project_paths(selected_project)

        # Display the selected configuration to the user
        print(f"\nUsing project configuration: {selected_project}")
        print(f"Ftrace file path: {FTRACE_FILE}")

        # Check if ftrace file exists
        if not check_file_exists(FTRACE_FILE):
            root = tk.Tk()
            root.withdraw()
            messagebox.showerror("File Not Found",
                                f"The ftrace file for {selected_project} was not found at:\n\n{FTRACE_FILE}\n\nPlease check if the file exists.")
            root.destroy()
            print(f"ERROR: The ftrace file for {selected_project} was not found at: {FTRACE_FILE}")
            print("Please check if the file exists and try again.")
            sys.exit(1)

        task_params = parse_ftrace_log(FTRACE_FILE)
        if task_params:
            cores_dict = organize_tasks_by_core(task_params)

            # Pass plot_type to print_formatted_output function
            print_formatted_output(task_params, plot_type)

            # Save to JSON file
            # Ensure the output directory exists
            os.makedirs(os.path.dirname(JSON_OUTPUT), exist_ok=True)
            save_cores_dict_to_json(cores_dict, JSON_OUTPUT)
            print(f"\nSuccessfully saved task parameters to: {JSON_OUTPUT}")

            # Only generate sample point images when dt or cet parameter is specified
            if plot_type == 'dt':
                plot_all_task_dt_samples(task_params, DT_PLOTS_DIR)
            elif plot_type == 'cet':
                plot_all_task_cet_samples(task_params, CET_PLOTS_DIR)
            else:
                print("\nSkip generating plots. Use 'python trace_data_parse.py dt' to generate DT plots or 'python trace_data_parse.py cet' to generate CET plots.")

        else:
            root = tk.Tk()
            root.withdraw()
            messagebox.showerror("Parse Error",
                               f"Failed to parse the ftrace file for {selected_project}.\n\nPlease check if the file format is correct.")
            root.destroy()
            print("Failed to parse task parameters")

    except Exception as e:
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror("Error", f"An error occurred:\n\n{str(e)}")
        root.destroy()
        print(f"Error in main program: {str(e)}")
        traceback.print_exc()