import os
#Path to the directory where the configuration file of each instance of ed are defined
INSTANCES_DIR = os.path.join(os.path.dirname(__file__), 'fixtures')

#Validate the presence of a mx record on the domain
EMAIL_CHECK_MX = False

#Validate the email by connecting to the smtp server, but doesn't send an email
EMAIL_CHECK_SMTP = False
