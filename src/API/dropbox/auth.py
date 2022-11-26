#!/usr/bin/env python3

import os
import dropbox
from dropbox import DropboxOAuth2FlowNoRedirect
#from dropbox import DropboxOAuth2Flow

# old exmaple: https://www.dropboxforum.com/t5/Dropbox-API-Support-Feedback/Python-sdk-error-DropboxAuth2Flow/td-p/257284

# dropbox docs: https://dropbox-sdk-python.readthedocs.io/en/latest/


# example code from: https://github.com/dropbox/dropbox-sdk-python/blob/master/example/oauth/commandline-oauth.py

def verify(token):
    with dropbox.Dropbox(oauth2_access_token=token) as dbx:
        dbx.users_get_current_account()
        print("Successfully set up client!")


APP_KEY = "ppbjb7aw7n67xpl"
APP_SECRET = "cr1505pqcbpttc0"


if os.path.exists('token.txt'):
    #creds = Credentials.from_authorized_user_file('token.json', SCOPES)
    f = open("token.txt", "r")
    token = f.readline()
    verify(token)

else:

    '''
    sesh = {}

    auth_flow = DropboxOAuth2Flow(consumer_key=APP_KEY, consumer_secret=APP_SECRET, session=sesh, csrf_token_session_key='dropbox-auth-csrf-token', redirect_uri ='http://localhost:8000/dropbox')
    auth_flow.start()
    '''

    auth_flow = DropboxOAuth2FlowNoRedirect(APP_KEY, APP_SECRET)

    authorize_url = auth_flow.start()
    print("1. Go to: " + authorize_url)
    print("2. Click \"Allow\" (you might have to log in first).")
    print("3. Copy the authorization code.")
    auth_code = input("Enter the authorization code here: ").strip()

    try:
        oauth_result = auth_flow.finish(auth_code)
    except Exception as e:
        print('Error: %s' % (e,))
        exit(1)

    print(oauth_result)


    f = open("token.txt", "w+")
    f.write(oauth_result.access_token)
    f.close()

    verify(oauth_result.access_token)

