import requests
import sqlite3


all_scores_db = '/var/jail/home/shengh/trivia_game/modified_score_sheet.db'
#all_scores_db = 'all_scores.db'

def get_response_from_api():
  """goes to the open trivia API and get only 1 true and false question."""
  to_send = "https://opentdb.com/api.php?amount=1&type=boolean"
  r = requests.get(to_send)
  data = r.json()
  
  question = data['results'][0]['question'].replace("&#039;", "'").replace("&quot;", "\"")
  correct_answer = data['results'][0]['correct_answer']
  
  return_string = '{}&{}'.format(question, correct_answer) 
  
  return return_string

def store_into_data_base(username, user_score, correct_answers, incorrect_answers):
  """first it deletes the item with the same username and store in the new data. 
  """
  conn = sqlite3.connect(all_scores_db)  # connect to that database (will create if it doesn't already exist)
  c = conn.cursor()  # move cursor into database (allows us to execute commands)
  c.execute('''CREATE TABLE IF NOT EXISTS scores_table (user text,score int, correct_answers int, incorrect_answers int);''') # run a CREATE TABLE command
  
  things = c.execute('''SELECT * FROM scores_table;''').fetchall()
  
  exist = 0
  for item in things:
    if item[0] == username:
      exist += 1
      break
  
  if exist == 0:
  
    c.execute('''INSERT into scores_table VALUES(?,?,?,?);''', (username, user_score, correct_answers, incorrect_answers))
    
  else:
    c.execute('''UPDATE scores_table SET score = ?, correct_answers = ?, incorrect_answers = ? WHERE user=?;''', (user_score, correct_answers, incorrect_answers, username))

  #PRINTING THINGS OUT / DEBUGGING PURPOSES  
  # things = c.execute('''SELECT * FROM scores_table;''').fetchall()
  # for x in things:
  #     print(x)
  
  conn.commit() # commit commands
  conn.close() # close connection to database

def get_data_decreasing():
  """returns a list of tuple that contains (username, score)s from the data base. 
  """
  conn = sqlite3.connect(all_scores_db)
  c = conn.cursor()
  things = c.execute('''SELECT * FROM scores_table ORDER BY score DESC;''')
  return things

def format_into_html(data_list):
  """takes in a list and return a html formatted unorder list
  """
  return_string = '<ol> <li> USER | SCORE | CORRECT ANSWERS | INCORRECT ANSWERS'
  
  for item in data_list:
    new_string = '<li> {} | {} | {} | {} </li>'.format(item[0], item[1], item[2], item[3])
    return_string += new_string
    
  return_string += '</ol>'
  return return_string
  
def get_user_data(username):
  """return specific user data from database
  """
  conn = sqlite3.connect(all_scores_db)
  c = conn.cursor()
  things = c.execute('''SELECT * FROM scores_table;''')
  for item in things:
    if item[0] == username:
      score = item[1]
      correct = item[2]
      incorrect = item[3]
      
      new_string = '{}&{}${}@'.format(score, correct, incorrect)
      return new_string
  
  return '0&0$0@'
  
  
def request_handler(request):
  """
  There is going to be 4 different types of requests
  
  1. request with scoreboard = True -> then we just want to return the html list
  
  2. request with scoreboard = False -> then we actually don't care
  
  3. request with no args -> getting the response from the API
  
  4. request with POST as method -> posting username and score to database 
  """
  if request['method'] == 'GET':
    if 'scoreboard' in request['args']:
      if request['values']['scoreboard'] == 'True':
        #get stuff from database
        #set into html format
        #return html code     
        decreasing_order = get_data_decreasing()
        html_format = format_into_html(decreasing_order)
        return html_format
    elif 'user' in request['args']:
      user_data = get_user_data(request['values']['user'])
      return user_data
    else:
      return get_response_from_api()
  else:
    user_name = request['values']['user']
    user_score = int(request['values']['score'])
    correct_answers = int(request['values']['correct'])
    incorrect_answer = int(request['values']['incorrect'])
    store_into_data_base(user_name, user_score, correct_answers, incorrect_answer)
    
  
      
      
# request = {'method': 'GET', 'args': ['scoreboard'], 'values': {'scoreboard': 'True'}}

# request_1 = {'method': 'GET', 'args': ['scoreboard'], 'values': {'scoreboard': 'False'}}


# request_2 = {'method': 'GET', 'args': [], 'values': {}}


# request_3 = {'method': 'POST', 'args': ['user', 'score'], 'values': {'user': 'Michael Jackson Ghost', 'score': '45'}}

# store_into_data_base('JACKSON_2', 40)

#print(request_handler(request))
#print(get_response_from_api())
# print(request_handler(request))
# print()
# print(request_handler(request_1))
# print()
# print(request_handler(request_2))

# request_handler(request_3)
# print(request_handler(request))
