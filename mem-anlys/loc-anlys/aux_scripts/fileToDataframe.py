import re
dictBlockIdLine={}
def get_intra_obj (data_intra_obj, fileline,reg_page_id,regionIdNum,numExtraPages:int=0):
    #print(reg_page_id,numExtraPages)
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
            add_row_index = 515+(int(cor_data[0])-255)
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
    #print(add_row)
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
            list_col_names[516+j] = '- [2^'+str((i-1))+'-2^'+str(i)+') p'
        j = j+1
    j=int(numExtraPages/2)
    for i in range (1,int(numExtraPages/2)+1):
        if( i ==1):
            list_col_names[516+j] = '+ 1p'
        elif (i<13):
            list_col_names[516+j] = '+ ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
            list_col_names[516+j] = '+ [2^'+str((i-1))+'-2^'+str(i)+') p'

        j = j+1
    list_col_names[516+numExtraPages]='Type'
    #print((list_col_names))
    return list_col_names

def getFileColumnNamesPageRegion (regionId, numRegions):
    # 517 for page, 10 pages in region, numRegions, 1 - non-hot, 1 - stack
    list_col_names=[None]*(517+10+numRegions+2)
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
    for i in range (0,10):
            list_col_names[516+i] = 'p-'+str(i)
    for i in range (0,numRegions):
            list_col_names[526+i] = 'r-'+str(i)
    list_col_names[526+numRegions]='Non-hot'
    list_col_names[526+numRegions+1]='Stack'
    list_col_names[526+numRegions+2]='Type'

    #print((list_col_names))
    return list_col_names

def getFileColumnNamesLineRegion (strFileName, numRegions):
    with open(strFileName) as f:
        for fileLine in f:
            if (fileLine.startswith('#---- Top Access line')):
                data = fileLine.replace('\n','').split(' ')
                #print(data[5], data[11],data[13],data[15])
                dictBlockIdLine[str(data[5])]=str(data[11])+'-'+str(data[13])+'-'+str(data[15])
    f.close()
    #for k, v in dictBlockIdLine.items():
    #    print(k, v)
    # 517 for page, 10 pages in region, numRegions, 1 - non-hot, 1 - stack
    list_col_names=[None]*(517+10+numRegions+2)
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
    for i in range (0,10):
            list_col_names[516+i] =  dictBlockIdLine[str(256+i)]
    for i in range (0,numRegions):
            list_col_names[526+i] = 'r-'+str(i)
    list_col_names[526+numRegions]='Non-hot'
    list_col_names[526+numRegions+1]='Stack'
    list_col_names[526+numRegions+2]='Type'
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
            metricColumns[511+j] = '- [2^'+str((i-1))+'-2^'+str(i)+') p'
        j = j+1
    j=int(numExtraPages/2)
    for i in range (1,int(numExtraPages/2)+1):
        if( i ==1):
            metricColumns[511+j] = '+ 1p'
        elif (i<13):
            metricColumns[511+j] = '+ ['+str(2**(i-1))+'-'+str((2**i)-1)+'] p'
        else:
           metricColumns[511+j] = '+ [2^'+str((i-1))+'-2^'+str(i)+') p'
        j = j+1
    return metricColumns

def getMetricColumnsPageRegion(regionId, numRegions):
    metricColumns=[None]*(511+10+numRegions+2)
    for i in range ( 0,255):
        metricColumns[i]='self'+'-'+str(255-i)
    metricColumns[255]='self'
    for i in range ( 1,256):
        metricColumns[255+i]='self'+'+'+str(i)
    for i in range (0,10):
            metricColumns[511+i] = 'p-'+str(i)
    for i in range (0,numRegions):
            metricColumns[521+i] = 'r-'+str(i)
    metricColumns[521+numRegions]='Non-hot'
    metricColumns[521+numRegions+1]='Stack'
    return metricColumns

def getMetricColumnsLineRegion(regionId, numRegions):
    metricColumns=[None]*(511+10+numRegions+2)
    for i in range ( 0,255):
        metricColumns[i]='self'+'-'+str(255-i)
    metricColumns[255]='self'
    for i in range ( 1,256):
        metricColumns[255+i]='self'+'+'+str(i)
    for i in range (0,10):
            metricColumns[511+i] = dictBlockIdLine[str(256+i)]
    for i in range (0,numRegions):
            metricColumns[521+i] = 'r-'+str(i)
    metricColumns[521+numRegions]='Non-hot'
    metricColumns[521+numRegions+1]='Stack'
    #metricColumns[521+1]='Stack'
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

def getPageColListPageRegion(listColNames):
    colList= listColNames
    pattern = re.compile('Non-hot')
    lowerPagelist=list(filter(pattern.match, colList))
    colRearrangeList=[]
    if(len(lowerPagelist)!=0):
        subList = [x for x in colList if x not in lowerPagelist]
        colRearrangeList.extend(subList)
    else:
        colRearrangeList.extend(listColNames)
    return colRearrangeList

def getPageColListLineRegion(listColNames):
    colList= listColNames
    pattern = re.compile('Non-hot')
    lowerPagelist=list(filter(pattern.match, colList))
    colRearrangeList=[]
    if(len(lowerPagelist)!=0):
        subList = [x for x in colList if x not in lowerPagelist]
        colRearrangeList.extend(subList)
    else:
        colRearrangeList.extend(listColNames)
    return colRearrangeList

def add_hot_lines (strFileName, strOutFile):
    dictRegions= {}
    dictTopLines={}
    f_out=open(strOutFile,'w')
    with open(strFileName) as f:
        lineStart='---'
        lineEnd='<----'
        flRegions = True
        flLines= False
        flNewLine=False
        for fileLine in f:
            flNewLine=False
            if(fileLine.startswith(lineStart) and flRegions):
                data = fileLine.replace('\n','').split(' ')
                #print(data[2], data[4])
                dictRegions[data[2]]= data[4]
            if (fileLine.startswith(lineEnd)):
                flRegions = False
            if (fileLine.startswith('#---- Top Access line')):
                data = fileLine.replace('\n','').split(' ')
                #print(data[5], data[11],data[13],data[15])
                #print(str(data[11])+'-'+str(data[13])+'-'+str(data[15]))
                dictTopLines[data[11]+'-'+data[13]+'-'+data[15]] = str(data[5])
            if(fileLine.startswith('<---- New intra-region starts')):
                flLines=True
            if(flLines and len(fileLine.strip()) != 0):
                flNewLine=False
                newFileLine=''
                data = fileLine.replace('\n','').split(' ')
                #print(data)
                regionId=data[2][:-1]
                pageId = data[2][-1]
                if(regionId in dictRegions):
                    regPageId = dictRegions[regionId]+'-'+pageId
                    #print(regPageId)
                    listLines = [(key, value) for key, value in dictTopLines.items() if key.startswith(regPageId)]
                    if(len(listLines) !=0 ):
                        #print('listlines ', listLines, regPageId, data[2], data[4])
                        hot_line_data= {}
                        for i in range(len(listLines)):
                            index=listLines[i][0].rindex('-')+1
                            #print('list lines data ', listLines[i][0], listLines[i][0][index:])
                            hotBlock = listLines[i][0][index:]
                            if(str(hotBlock+',') in fileLine):
                                #print ('fileLine ', fileLine)
                                startIndex=fileLine.index(hotBlock+',')
                                lineSegment =fileLine[startIndex:]
                                startIndex = lineSegment.index(' ')
                                lineSegment = lineSegment[0:startIndex]
                                #print('lineSegment ', lineSegment)
                                cor_data=lineSegment.split(',')
                                #print( 'cor_data ', regPageId+'-'+cor_data[0])
                                if (str(regPageId+'-'+cor_data[0]) in dictTopLines):
                                    hot_line_data[dictTopLines[str(regPageId+'-'+cor_data[0])]] = cor_data[1]
                                    #print('yes', cor_data[0], cor_data[1])
                                #else:
                                    #print("NO ", str(regPageId+'-'+cor_data[0]))
                        #print('hot_line_data ',hot_line_data)
                        newFileLine=fileLine
                        #print('newFileLine ', newFileLine)
                        for (key, value ) in hot_line_data.items():
                            nextBlock = 0
                            flIndex=False
                            while (True):
                                nextBlock = nextBlock+1
                                searchBlock = int(key)+nextBlock
                                searchBlock = str(searchBlock)+','
                                #print(searchBlock)
                                if(newFileLine.find(searchBlock)):
                                    startIndex = newFileLine.find(searchBlock)
                                    if(startIndex != -1):
                                        #print(key, value, newFileLine)
                                        newFileLine = newFileLine[0:startIndex-1] + ' '+key+','+value+ ' ' +newFileLine[startIndex:]
                                        #print(newFileLine)
                                        #print(listLines, regPageId, data[2], data[4])
                                        flNewLine=True
                                        break
                                if(int(key)+nextBlock >=267):
                                    break
                            #print('after while ', startIndex)
            if(flNewLine == False):
                f_out.write(fileLine)
            else:
                f_out.write(newFileLine)

    f_out.close()
    f.close()
    print('input file ', strFileName)
    print('Fill in hot line affinity for each line - data from self+, self- repeated for blocks in same page')
    print ('output file ', strOutFile)

