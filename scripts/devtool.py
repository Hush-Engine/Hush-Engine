"""
devtool is a cli for developing the project, it contains
commands for adding new files, building, testing, etc.
"""

import datetime
import click
import jinja2

@click.group()
def cli():
  """
  devtool is a cli for developing the UNNAMED ENGINE project, it contains
  commands for adding new files, building, testing, etc.
  """
  pass


def check_cmake_installed():
  """
  Checks if CMake is installed.
  """
  import subprocess
  try:
      subprocess.check_call(['cmake', '--version'], stdout=subprocess.DEVNULL)
  except OSError:
      raise click.ClickException('CMake is not installed. Please install CMake and try again.')
    
def check_cmake_version():
  """
  Checks if CMake version is at least 3.25
  """
  import subprocess
  import re
  try:
      cmake_version = subprocess.check_output(['cmake', '--version']).decode('utf-8')
  except OSError:
      raise click.ClickException('CMake is not installed. Please install CMake and try again.')
  cmake_version = re.search(r'version\s+(\d+\.\d+\.\d+)', cmake_version).group(1)
  if cmake_version < '3.25':
      raise click.ClickException('CMake version must be at least 3.1. Please update CMake and try again.')
      
def check_ninja_installed():
  """
  Checks if Ninja is installed. If not installed, recommends installing Ninja.
  """
  import subprocess
  try:
    subprocess.check_call(['ninja', '--version'], stdout=subprocess.DEVNULL)
    return True
  except OSError:
    return False


@click.command()
@click.option('--build-type', '-t', default='Release', help='Build type (Debug, Release, RelWithDebInfo, MinSizeRel)')
@click.option('--build-dir', '-b', default='build', help='Build directory')
@click.option('--no-echo', '-n', is_flag=True, help='Do not echo commands')
def configure(build_type, build_dir, no_echo):
  """
  Configures the project using CMake.
  """
  import os
  import subprocess
  
  click.echo('🛠️ Configuring project...')
  
  # Check if CMake is installed
  check_cmake_installed()
  check_cmake_version()
  
  # Check if Ninja is installed
  is_ninja_installed = check_ninja_installed()
  if not is_ninja_installed:
    click.echo('⚠️ Ninja is not installed. Ninja is the recommended build system')

  # Configure
  os.makedirs(build_dir, exist_ok=True)
  os.chdir(build_dir)
  cmake_args = [f'-DCMAKE_BUILD_TYPE={build_type}']
  if is_ninja_installed:
    cmake_args.append('-GNinja')
  # For Windows, use Visual Studio 17 2022 or newer
  elif os.name == 'nt':
    cmake_args.append('-GVisual Studio 17 2022')
  elif os.name == 'posix':
    cmake_args.append('-GUnix Makefiles')
  # For OS X, use Xcode
  elif os.name == 'mac':
    cmake_args.append('-GXcode')
  cmake_args.append('..')
  
  subprocess.check_call(['cmake'] + cmake_args, stdout=None if no_echo else subprocess.DEVNULL)
  
  click.echo('✅ Project configured successfully.')
  
@click.command()
@click.option('--build-dir', '-b', default='build', help='Build directory')
@click.option('--no-echo', '-n', is_flag=True, help='Do not echo commands')
def build(build_dir, no_echo):
  """
  Builds the project using CMake.
  """
  import os
  import subprocess
  
  click.echo('🛠️ Building project...')
  
  # Check if CMake is installed
  check_cmake_installed()
  check_cmake_version()
  
  # Check if Ninja is installed
  is_ninja_installed = check_ninja_installed()
  if not is_ninja_installed:
    click.echo('⚠️ Ninja is not installed. Ninja is the recommended build system')

  # Build
  os.chdir(build_dir)
  if is_ninja_installed:
    subprocess.check_call(['ninja'], stdout=None if no_echo else subprocess.DEVNULL)
  else:
    subprocess.check_call(['cmake', '--build', '.'], stdout=None if no_echo else subprocess.DEVNULL)
  
  click.echo('✅ Project built successfully.')
  

def get_git_username():
  """
  Gets the git username from the git config.
  """
  import subprocess
  try:
    git_username = subprocess.check_output(['git', 'config', 'user.name']).decode('utf-8').strip()
  except OSError:
    raise click.ClickException('Git is not installed. Please install Git and try again.')
  return git_username

@click.command('new-file')
@click.option('--file-path', '-f', help='File path', required=False, type=click.Path())
@click.option('--brief', '-b', help='Brief description of the file', required=False)
def add(file_path, brief):
  """
  Adds a new file to the project.
  """
  import os
  import shutil

  if not file_path:
    #We'll prompt a file dialog if no path was provided
    file_path = save_with_file_dialog()
  # Check extension
  file_type = os.path.splitext(file_path)[1]
  if file_type == '':
    click.echo('⚠️ No file type specified, an empty file will be created.')
    with open(file_path, 'w') as f:
      pass
    return
  # Remove the dot
  file_type = file_type[1:]
  
  template_path = os.path.join(os.path.dirname(__file__), 'templates', f'.{file_type}')
  if not os.path.exists(template_path):
    raise click.ClickException(f'File type {file_type} is not supported.')
  
  # File_path includes the full path to the file
  if not os.path.exists(file_path):
    # Check if the directory exists
    directory = os.path.dirname(file_path)
    if not os.path.exists(directory) and directory != '':
      os.makedirs(directory)
    
    # Replace template variables
    author = get_git_username()
    date = datetime.datetime.now().strftime('%Y-%m-%d')
    if brief is None:
      brief = click.prompt('Brief description of the file')
    
    environment = jinja2.Environment(loader=jinja2.FileSystemLoader(os.path.dirname(template_path)))
    template = environment.get_template(os.path.basename(template_path))
    # Render filename, author, date, and brief
    click.echo(f'{date}')
    rendered = template.render(filename=os.path.basename(file_path), author=author, date=date, brief=brief)

    with open(file_path, 'w') as f:
      f.write(rendered)
    
    click.echo(f'✅ File {os.path.relpath(file_path)} created successfully.')
  else:
    raise click.ClickException(f'File {file_path} already exists.')

def save_with_file_dialog():
  import tkinter as tk
  from tkinter import filedialog
  import os

  dialogTitle = "DevTool - New file from template"
  parentDir = get_project_root()
  root = tk.Tk()
  root.withdraw()

  filePath = filedialog.asksaveasfilename(title=dialogTitle, initialdir=parentDir, defaultextension='*.*')

  if not filePath:
    raise click.ClickException(f'File creation was cancelled by the user!')

  return filePath


@click.command('update-links')
def update_linking():
  """ 
  Updates all Cmake lists to link .hpp and .cpp files.
  """
  raise click.ClickException(f'Update linking not implemented yet')

@click.command('docs')
@click.option('--open', '-o', is_flag=True ,help='Opens the index of the currently generated documentation')
@click.option('--make', '-m', is_flag=True, help='Generate or update the documentation')
@click.option('--delete', '-d', is_flag=True, help='Deletes all documentation files (for debug purposes)')
def handle_docs(open, make, delete):
  import os, subprocess

  if not open and not make and not delete:
    #Empty case
    raise click.ClickException("You must provide an option to the docs command, please see docs --help to learn more...")
  
  DOCS_FOLDER = '/docs/doxygen'
  PROJECT_ROOT = get_project_root()
  fullPath = PROJECT_ROOT + DOCS_FOLDER

  if open:
    click.echo('Opening the documentation...')
    #Go to docs/doxygen/index.html and open that file  
    indexPath = os.path.join(fullPath, 'html/index.html')
    if not os.path.exists(indexPath):
      raise click.ClickException('The documentation is not generated, we could not find ' + indexPath)
    subprocess.call(('open', indexPath))
  elif make:
    configFilePath = os.path.join(PROJECT_ROOT, 'Doxyfile.in')
    click.echo("Creating/Updating the documentation based on config file: " + configFilePath)
    #Create the docs/doxygen directories
    try:
      #Run doxygen Doxyfile.in
      os.system('doxygen ' + configFilePath)
    except FileNotFoundError as e:
      raise click.ClickException(e)
  elif delete:
    comfirmed = click.confirm('Are you sure you want to delete all documentation files?')
    if not comfirmed:
      click.echo('Cancelled delete operation')
      return
    click.echo('Deleting files...')
    for root, dirs, files in os.walk(fullPath):
      with click.progressbar(files, len(files)) as bar:
        for file in bar:
          os.remove(os.path.join(root, file))
    click.echo('Done deleting files c:')

def get_project_root():
  import os
  scripts = os.path.dirname(os.path.realpath(__file__))
  return os.path.dirname(scripts)

if __name__ == '__main__':
  cli.add_command(configure)
  cli.add_command(build)
  cli.add_command(add)
  cli.add_command(update_linking)
  cli.add_command(handle_docs)
  cli()