import requests
from bs4 import BeautifulSoup as bs
link='https://www.amazon.in/Apple-iPhone-13-128GB-Blue/product-reviews/B09G9BL5CP/ref=cm_cr_dp_d_show_all_btm?ie=UTF8&reviewerType=all_reviews'
page= requests.get(link)
page
page.content
soup=bs(page.content,'html.parser')
print(soup.prettify())
name=soup.select('span.a-profile-name')
name[2:]
cust=[]
for i in range(0,len(name)):
    cust.append(name[i].get_text())
rev=soup.select('span.review-date')
rev
reve=[]
for i in range(0,len(rev)):
    reve.append(rev[i].get_text())
reve[2:]
rat=soup.select('i.review-rating')
rat
rate=[]
for i in range(0,len(rat)):
    rate.append(rat[i].get_text())
rate[2:]
dat=soup.select('span.review-text-content')
data=[]
for i in range(0,len(dat)):
    data.append(dat[i].get_text())
data
import pandas as pd
df=pd.DataFrame()
df['cust']=name
df
