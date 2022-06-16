[demonstration video](https://youtu.be/JVsWN_9uA94)
# Overview

The state machine for this project is pretty simple. I will go through it quick, but the main part is at the server side.

# -------------------------------------------------------------------
So the game starts in **IDLE** and the user can press a button to begin the game. 

**IDLE** only leads into **START GAME** where the game basically prepares the game and it takes in the *harcoded* username in the arduino file and check against the database to see if data for that username exists. If it does then it gets that data and prepares the game to start.

# -------------------------------------------------------------------

In **PLAY GAME** the user has 3 options. Either they pick **TRUE**, **FALSE**, or if they want to stop playing the game, they can **POST** their score to the database and the databse gets updated with their new *score*, *correct answers*, and *incorrect answers*. 

**TRUE** and **FALSE** always leads back into **PLAY GAME** so that the user, technically, can keep on playing forever. 

The game will only end if the user decides to stop playing and post his or her score using the *button at port 38*. When this happens, the game resets back to the **IDLE** state and waits for the user to want to start playing the game again. 

# -------------------------------------------------------------------

The game can go through multiple power cycles because the data is not stored locally on the arduino. It's being retrieved from the database that the python file has made.

I have included some pictures of the different game screens. All of the instructions of how to operate the game is also printed on the TFT. 

# -------------------------------------------------------------------

# SERVER SIDE (HELPER FUNCTIONS)

There are several *helper functions* that I made to make the code more readable. 

The first one is called `get_data_decreasing()` which just get all the data from the database and list them in decreasing score order. 

The second is called `format_into_html(data_list)` which takes in a list argument and returns a  *html ordered list* format so that the web browser can read it. 

The third is called `get_user_data(username)` which takes in a username and checks to see if a username is in the database already. If it is, then we return the corresponding data. If it is not in the database, then we create a query for it and return all 0's as its data. 

The output format for `get_user_data(username)` looks like this: *score&correctanswers$incorrectanswers@* for easier processing on the arduino side. 

The fourth one is called `get_response_from_api()` which returns a question and its correct answer that we got from the api. 

The output format looks like this: *question&correctanswer*. 
# -------------------------------------------------------------------

# Request handler

So there are basically 4 cases to which we want to think about for request handler. 

1. request with *scoreboard* = *True* -> then we just want to return the html list
  
2. request with *scoreboard* = *False* -> then we actually don't care
  
3. request with no args -> getting the response from the API, questions and answer
  
4. request with POST as method -> posting username and score to database

And since we have written the helper functions, we can just use those in the request handler. 



