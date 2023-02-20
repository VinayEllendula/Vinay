#!/usr/bin/env python
# coding: utf-8

# In[139]:


import requests
from bs4 import BeautifulSoup as bs


# In[39]:


link='https://www.amazon.in/Apple-iPhone-13-128GB-Blue/product-reviews/B09G9BL5CP/ref=cm_cr_dp_d_show_all_btm?ie=UTF8&reviewerType=all_reviews'


# In[40]:


page= requests.get(link)
page


# In[41]:


page.content


# In[141]:


soup=bs(page.content,'html.parser')


# In[142]:


print(soup.prettify())


# In[143]:


name=soup.select('span.a-profile-name')


# In[144]:


name[2:]


# In[145]:


cust=[]
for i in range(0,len(name)):
    cust.append(name[i].get_text())


# In[ ]:





# In[146]:


rev=soup.select('span.review-date')


# In[147]:


rev


# In[148]:


reve=[]
for i in range(0,len(rev)):
    reve.append(rev[i].get_text())
    


# In[149]:


reve[2:]


# In[150]:


rat=soup.select('i.review-rating')


# In[151]:


rat


# In[152]:


rate=[]
for i in range(0,len(rat)):
    rate.append(rat[i].get_text())


# In[153]:


rate[2:]


# In[154]:


dat=soup.select('span.review-text-content')


# In[155]:


data=[]
for i in range(0,len(dat)):
    data.append(dat[i].get_text())
data


# In[156]:


import pandas as pd


# In[157]:


df=pd.DataFrame()


# In[158]:


df['cust']=name


# In[159]:


df


# In[ ]:





# In[ ]:





# In[ ]:




