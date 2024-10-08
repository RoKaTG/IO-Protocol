import os  # Import the os module for interacting with the operating system, such as file paths
import sys  # Import the sys module to handle command-line arguments
import json  # Import the json module to parse JSON files
import pandas as pd  # Import the pandas library for data manipulation and analysis
import matplotlib.pyplot as plt  # Import the matplotlib library for creating plots

# Check if the correct number of command-line arguments is provided
if len(sys.argv) != 2:
    print("Usage: python box_plot_baseline.py <log_dir>")  # Display usage instructions
    sys.exit(1)  # Exit the program if the wrong number of arguments is provided

# Retrieve the path to the log directory from the command-line argument
log_dir = sys.argv[1]

# Define the path to the baseline JSON file within the log directory
json_file = os.path.join(log_dir, 'baseline', 'baseline.json')

# Define the directory where the boxplot will be saved
boxplot_dir = os.path.join(log_dir, 'box_plot')
os.makedirs(boxplot_dir, exist_ok=True)  # Create the directory if it doesn't exist

# Load the JSON data from the baseline file
with open(json_file, 'r') as f:
    data = json.load(f)

# Convert the JSON data into a pandas DataFrame for easier manipulation
df = pd.json_normalize(data)

# Filter the DataFrame to include only the baseline measurements
df_baseline = df[df['metric_id'] == 'wattmetre_power_watt']

# Add a new column to the DataFrame for labeling the boxplot
df_baseline['label'] = 'baseline'

# Create the boxplot using the filtered baseline data
plt.figure(figsize=(10, 6))
boxplot = df_baseline.boxplot(column='value', by='label', grid=True, showfliers=False, patch_artist=True)

# Customize the appearance of the boxplot
for patch in boxplot.artists:
    patch.set_facecolor('purple')  # Set the fill color of the boxplot to purple
    patch.set_edgecolor('black')   # Set the edge color of the boxplot to black

# Add labels and a title to the boxplot
plt.title('Boxplot of wattmeter measurement during 15 minutes before IO')
plt.suptitle('')  # Remove the automatic subtitle generated by pandas
plt.xlabel('')  # Remove the x-axis label
plt.ylabel('Watt')  # Label the y-axis as "Watt"
plt.xticks(rotation=0)  # Keep the x-axis labels horizontal

# Add a grid to the boxplot for better readability
plt.grid(True, linestyle='--', linewidth=0.7, alpha=0.7)

# Save the boxplot as a PNG image in the specified directory
output_file = os.path.join(boxplot_dir, 'boxplot_baseline.png')
plt.savefig(output_file)

# Close the plot to prevent additional empty plots from being displayed
plt.close()

# Print a message indicating that the boxplot has been saved
print(f"Boxplot saved to {output_file}")

