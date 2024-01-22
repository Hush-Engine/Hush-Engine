"""
devtool is a cli for developing the project, it contains
commands for adding new files, building, testing, etc.
"""

import datetime
from sys import stderr, stdout
import click
import jinja2
import subprocess
import os

OS_WINDOWS_ID = 'nt'
OS_MAC_ID = 'mac'
OS_UNIX_ID = 'posix'

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
  try:
      subprocess.check_call(['cmake', '--version'], stdout=subprocess.DEVNULL)
  except OSError:
      raise click.ClickException('CMake is not installed. Please install CMake and try again.')

def check_cmake_version():
  """
  Checks if CMake version is at least 3.25
  """
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
  elif os.name == OS_WINDOWS_ID:
    cmake_args.append('-GVisual Studio 17 2022')
  elif os.name == OS_UNIX_ID:
    cmake_args.append('-GUnix Makefiles')
  # For OS X, use Xcode
  elif os.name == OS_MAC_ID:
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
@click.option('--verbose', '-v', is_flag=True, help='Shows all output')
def handle_docs(open, make, delete, verbose):
  import os, subprocess

  if not open and not make and not delete:
    #Empty case
    raise click.ClickException("You must provide an option to the docs command, please see docs --help to learn more...")
  #Check if on windows (Yeah, I know we could use path, but for some reason it's shortening to a relative one when we want the absolute path)

  DOCS_FOLDER = '\\docs\\build' if os.name == OS_WINDOWS_ID else '/docs/build'
  PROJECT_ROOT = get_project_root()
  fullPath = PROJECT_ROOT + DOCS_FOLDER

  if open:
   open_generated_docs(fullPath)
  elif make:
    generate_docs(PROJECT_ROOT, verbose=verbose)
  elif delete:
    delete_docs(fullPath)

@click.command('format')
@click.option('--verbose', '-v', is_flag=True, help='Shows all output')
def format(verbose: bool):
  """
  Formats the project using clang-format.
  """

  # Check if clang-format is installed
  try:
    subprocess.check_call(['clang-format', '--version'], stdout=subprocess.DEVNULL)
  except OSError:
    raise click.ClickException('clang-format is not installed. Please install clang-format and try again.')

  sources = []
  for root, dirs, files in os.walk('src'):
    for file in files:
      if file.endswith('.cpp') or file.endswith('.hpp'):
        sources.append(os.path.join(root, file))

  if len(sources) == 0:
    raise click.ClickException('No source files found.')

  subprocess.check_call(['clang-format', '-style=file', '-i'] + sources, stdout=None if verbose else subprocess.DEVNULL, stderr=None if verbose else subprocess.DEVNULL)

  click.echo('✅ Project formatted successfully.')

@click.command('tidy')
@click.option('--verbose', '-v', is_flag=True, help='Shows all output')
@click.option('--fix', '-f', is_flag=True, help='Fixes the issues')
def tidy(verbose: bool, fix: bool):
  """
  Checks the project for issues using clang-tidy.
  """

  # Check if clang-tidy is installed
  try:
    subprocess.check_call(['clang-tidy', '--version'], stdout=subprocess.DEVNULL)
  except OSError:
    raise click.ClickException('clang-tidy is not installed. Please install clang-tidy and try again.')

  CONFIG_FILE = '.clang-tidy'
  WRAPPER_FILE = './scripts/run-clang-tidy.py'

  # Check the python executable
  python_executable = 'python'
  if os.name == OS_WINDOWS_ID:
    python_executable = 'python.exe'
  elif os.name == OS_MAC_ID or os.name == OS_UNIX_ID:
    python_executable = 'python3'

  sources = []
  for root, dirs, files in os.walk('src'):
    for file in files:
      if file.endswith('.cpp') or file.endswith('.hpp'):
        sources.append(os.path.join(root, file))

  if len(sources) == 0:
    raise click.ClickException('No source files found.')

  args = [python_executable, WRAPPER_FILE, '-p', 'build', './src', '-quiet', '-header-filter=.*']
  if fix:
    args.append('-fix')

  click.echo('🛠️ Checking project for issues...')
  exit_code = subprocess.check_call(args, stdout=None if verbose else subprocess.DEVNULL, stderr=None if verbose else subprocess.DEVNULL)

  if exit_code == 0:
    message = 'clang-tidy found issues.'
    if fix:
      message += ' clang-tidy fixed the issues.'
    if not verbose:
      message += ' Run with --verbose for more information.'
    raise click.ClickException(message)
  click.echo('✅ Project checked successfully.')

def open_generated_docs(pathToBuild):
  import os, subprocess
  click.echo('Opening the documentation...')
  #Go to docs/doxygen/index.html and open that file
  indexPath = os.path.join(pathToBuild, 'html\\index.html' if os.name == OS_WINDOWS_ID else 'html/index.html')
  if not os.path.exists(indexPath):
    raise click.ClickException('The documentation is not generated, we could not find ' + indexPath)
  if os.name == OS_WINDOWS_ID:
    os.startfile(indexPath)
  else:
    subprocess.call(('open', indexPath))

def generate_docs(pathToProjectRoot, verbose=False):
  click.echo("Creating/Updating the documentation based on config file Doxyfile.in")
  #Create the docs/doxygen directories
  outputStream = None if verbose else subprocess.DEVNULL
  try:
    configFilePath = 'Doxyfile.in'
    #Run doxygen Doxyfile.in
    returnCode = subprocess.Popen(['doxygen', configFilePath], cwd=pathToProjectRoot, shell=True, stdout=outputStream, stderr=outputStream).wait()
    if returnCode != 0:
      raise click.ClickException('Failed to run doxygen, please verify your input')
    else:
      click.echo('Finished generating doxygen docs, please wait while we run formatting with sphinx...')
    #Now make the sphynx docs by calling make
    if os.name == OS_WINDOWS_ID:
      #TODO: This still does not work with sphinx and Windows, so, yeah
      makeTargetDir = pathToProjectRoot + '\\docs'
      baseSphinxBuildCmd = 'python -m sphinx.cmd.build'
      sphinxCmd = baseSphinxBuildCmd + '>NUL 2>NUL'
      returnCode = subprocess.call(sphinxCmd, shell=True, stdout=outputStream, stdin=outputStream)
      sphinxCmd = 'python -m sphinx.cmd.build -M ' + makeTargetDir + '\\source ' + makeTargetDir + '\\build html'
      returnCode = subprocess.call(sphinxCmd, shell=True, stdout=outputStream, stdin=outputStream)
      if returnCode != 0:
        raise click.ClickException('There was an error running Sphinx, please verify your path and installation. Exit code ' + str(returnCode))
    else:
      makeTargetDir = pathToProjectRoot + '/docs'
      subprocess.call('make html -C ' + makeTargetDir, shell=True)
    #Alert the user
    click.echo('Finished generating the documentation, run docs -o to open the index file')
  except FileNotFoundError as e:
    message = e.strerror
    if e.filename:
      message += str(e.filename)
    raise click.ClickException(message)

def delete_docs(pathToBuild):
  comfirmed = click.confirm('Are you sure you want to delete all documentation files?')
  if not comfirmed:
    click.echo('Cancelled delete operation')
    return
  click.echo('Deleting files...')
  for root, dirs, files in os.walk(pathToBuild):
    with click.progressbar(files, len(files)) as bar:
      for file in bar:
        os.remove(os.path.join(root, file))
  click.echo('Done deleting files c:')

def get_project_root():
  import os
  scripts = os.path.dirname(os.path.realpath(__file__))
  return os.path.dirname(scripts)

def get_parent_dir(dir):
  import os
  return os.path.dirname(os.path.realpath(dir))

if __name__ == '__main__':
  cli.add_command(configure)
  cli.add_command(build)
  cli.add_command(add)
  cli.add_command(update_linking)
  cli.add_command(handle_docs)
  cli.add_command(format)
  cli.add_command(tidy)
  cli()
