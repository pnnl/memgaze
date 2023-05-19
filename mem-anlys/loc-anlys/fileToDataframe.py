import re
def get_intra_obj (data_intra_obj, fileline,reg_page_id,regionIdNum,numExtraPages:int=0):
    #print(reg_page_id)
    add_row=[None]*(517+numExtraPages)
    data = fileline.strip().split(' ')
    #print("in fill_data_frame", data[2], data[4], data[15:])
    str_index=regionIdNum+'-'+data[2][-1]+'-'+data[4] #regionid-pageid-cacheline
    add_row[0]=reg_page_id
    add_row[1]=str_index
    add_row[2] = data[11] # access count
    add_row[3]=data[9] # lifetime
    add_row[4]=data[7] # Address range
    linecache = int(data[4])
    # Added value for 'self' so that it doesnt get dropped in dropna
    add_row[260]=0.0
    for i in range(15,len(data)):
        cor_data=data[i].split(',')
        #print(cor_data)
        #if(int(cor_data[0])<256):
        if(int(cor_data[0])<=linecache):
            add_row_index=5+255-(linecache-int(cor_data[0]))
            #print("if ", linecache, cor_data[0], 5+255-(linecache-int(cor_data[0])))
        else:
            add_row_index=5+255+(int(cor_data[0])-linecache)
        #print("else", linecache, cor_data[0], 5+255+(int(cor_data[0])-linecache))
        if(int(cor_data[0])>255):
            add_row_index = 516+(int(cor_data[0])-255)
            #print('pages line', data[0], 'reg-page-id ', reg_page_id, ' access ', data[11], cor_data[0], cor_data[1],  ' index ', add_row_index)
        add_row[add_row_index]=cor_data[1]
        #if(add_row_index == 260 ):
            #print('address', add_row[4], 'index', add_row_index, ' value ', add_row[add_row_index])
    if(data[0] == '==='):
        add_row[516+numExtraPages] = 'SP'
    elif(data[0] == '---'):
        add_row[516+numExtraPages] = 'SI'
    elif(data[0] == '***'):
        add_row[516+numExtraPages] = 'SD'
    data_intra_obj.append(add_row)

def getFileColumnNames (numExtraPages:int =0):
    list_col_names=[None]*(517+numExtraPages)
    list_col_names[0]='reg_page_id'
    list_col_names[1]='reg-page-blk'
    list_col_names[2]='Access'
    list_col_names[3]='Lifetime'
    list_col_names[4]='Address'
    for i in range ( 0,255):
        list_col_names[5+i]='self'+'-'+str(255-i)
    list_col_names[260]='self'
    for i in range ( 1,256):
        list_col_names[260+i]='self'+'+'+str(i)
    j=0
    for i in range (int(numExtraPages/2),0,-1):
        if(i==1):
            list_col_names[516+j] = '- 1p'
        elif(i<13):
            list_col_names[516+j] = '- ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
            list_col_names[516+j] = '- [2^'+str((i-1))+'-2^'+str(i)+'] p'
        j = j+1
    j=int(numExtraPages/2)
    for i in range (1,int(numExtraPages/2)+1):
        if( i ==1):
            list_col_names[516+j] = '+ 1p'
        elif (i<13):
            list_col_names[516+j] = '+ ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
            list_col_names[516+j] = '+ [2^'+str((i-1))+'-2^'+str(i)+'] p'

        j = j+1
    list_col_names[516+numExtraPages]='Type'
    #print((list_col_names))
    return list_col_names

def getMetricColumns(numExtraPages:int =0):
    metricColumns=[None]*(511+numExtraPages)
    for i in range ( 0,255):
        metricColumns[i]='self'+'-'+str(255-i)
    metricColumns[255]='self'
    for i in range ( 1,256):
        metricColumns[255+i]='self'+'+'+str(i)
    j=0
    for i in range (int(numExtraPages/2),0,-1):
        if( i ==1):
            metricColumns[511+j] = '- 1p'
        elif(i<13):
            metricColumns[511+j] = '- ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
            metricColumns[511+j] = '- [2^'+str((i-1))+'-2^'+str(i)+'] p'
        j = j+1
    j=int(numExtraPages/2)
    for i in range (1,int(numExtraPages/2)+1):
        if( i ==1):
            metricColumns[511+j] = '+ 1p'
        elif (i<13):
            metricColumns[511+j] = '+ ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
           metricColumns[511+j] = '+ [2^'+str((i-1))+'-2^'+str(i)+'] p'
        j = j+1
    return metricColumns

def getRearrangeColumns(listColNames):
    colList= listColNames
    pattern = re.compile('-.*p')
    lowerPagelist=list(filter(pattern.match, colList))
    pattern = re.compile('\+.*p')
    upperPagelist=list(filter(pattern.match, colList))
    selfStartIndex = [colList.index(l) for l in colList if l.startswith('self-')]
    #print(selfStartIndex[0])
    colRevList = list(reversed(colList))
    #print(colRevList)
    selfEndIndex = [colRevList.index(l) for l in colRevList if l.startswith('self+')]
    #print(len(colList)-selfEndIndex[0]-1)
    colRearrangeList=colList[:selfStartIndex[0]]
    colRearrangeList.extend(lowerPagelist)
    colRearrangeList.extend(colList[selfStartIndex[0]: (len(colList)-selfEndIndex[0])])
    colRearrangeList.extend(upperPagelist)
    colRearrangeList.append(colList[-1])
    return colRearrangeList

def getPageColList(listColNames):
    colList= listColNames
    pattern = re.compile('-.*p')
    lowerPagelist=list(filter(pattern.match, colList))
    pattern = re.compile('\+.*p')
    upperPagelist=list(filter(pattern.match, colList))
    pattern = re.compile('self.*')
    selfList=list(filter(pattern.match, colList))
    colRearrangeList=[]
    colRearrangeList.extend(lowerPagelist)
    colRearrangeList.extend(selfList)
    colRearrangeList.extend(upperPagelist)
    return colRearrangeList
