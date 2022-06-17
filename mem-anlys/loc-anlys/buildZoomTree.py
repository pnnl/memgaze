import graphviz
levelOneArray=[]
levelTwoArray=[]
levelThreeArray=[]
levelArrays=[[] for _ in range(11)]
dictChildParent = {}
dictCombParent ={}
dictAggChild ={}
arrBlockSize=[]

def readFile(filename, version):
    listChildNode=[]

    with open(filename) as f:
        #arrBlockSize.append('Whole Trace')
        for fileLine in f:
            childAccessRUD=''
            childPages=''
            flCombineChildren=0
            oneChildName=''
            level=''
            if not('skip this due to small' in fileLine):
                data = fileLine.strip().split(' ')
                #1 Level 2 ID A Parent R
                if 'Level' in data[1]: # Level
                    level=data[2]
                if 'ID' in data[3]:
                    nodeID= data[4]
                for  i in range(5,len(data)):
                    j = i
                    if 'Parent' in data[i]:
                        parentNode = data[i+3].replace('(','').replace(')','').split(',')
                        leftParent = int(parentNode[0])
                        rightParent = int(parentNode[1])
                        if(leftParent != rightParent):
                            print (leftParent, rightParent)
                            for k in range(leftParent, rightParent):
                                print('k ', k)
                                parentEdge=str(int(level)-1)+data[i+1]+'_p'+str(k)
                                #print(parentEdge)
                                dictCombParent[parentEdge] = str(int(level)-1)+data[i+1]+'_p'+str(k+1)
                                k = k+1
                        parentNodeName=data[i+1]+'_p'+parentNode[0]
                    if 'Block' in data[i]:
                        blockSize=data[i+2]
                        if (not(blockSize in arrBlockSize)):
                            arrBlockSize.append(blockSize)
                    if int(j)>= 14 and j<=38:
                        if 'p' in data[i] and 'pc' not in data[i]:
                            childNodeName = level+nodeID+'_'+data[i].strip(':')
                            #childNodeLabel = data[i].strip(':')+'\n'+ data[i+1].replace('-','\n')+'\n'+data[i+2].strip(';').replace(')','').replace('(','\n')
                            childNodeLabel = nodeID+'_'+data[i].strip(':').replace('p','')+'\n'+data[i+2].strip(';').replace(')','').replace('(','\n')
                            if j==38:
                                childNodeLabel = '......'
                            listChildNode = [childNodeName, childNodeLabel]
                            if int(level) == 1:
                                levelOneArray.append(listChildNode)
                                dictChildParent[childNodeName] = 'Root'
                                levelArrays[0].append(listChildNode)
                            if int(level) >= 2:
                                #print('level', level)
                                dictChildParent[childNodeName] = str(int(level)-1)+parentNodeName
                                #print (listChildNode)
                                levelArrays[int(level)-1].append(listChildNode)
                        elif 'pc' in data[i]:
                                flCombineChildren =flCombineChildren+1
                                oneChildName = level+nodeID+'_'+data[i].strip(':')
                                combineChildName = data[i].strip(':').replace('pc','')+ ' - '
                                childPages= childPages+combineChildName.rstrip(combineChildName[-1])
                                combineChildRUD = data[i+1].strip(';')
                                combineChildRUD= combineChildRUD.replace('(', ' (')
                                childAccessRUD = childAccessRUD+combineChildName+combineChildRUD+'\n'
                if(flCombineChildren>=1):
                    if(flCombineChildren==1):
                        childNodeName = oneChildName
                        childNodeLabel = combineChildRUD.replace(' (','\n').replace(')','\n')
                    else:
                        childNodeName = 'comb'+level+nodeID+childPages.replace(' ','')
                        childNodeLabel = childAccessRUD
                    listChildNode = [childNodeName, childNodeLabel]

                    dictChildParent[childNodeName] = str(int(level)-1)+parentNodeName
                    levelArrays[int(level)-1].append(listChildNode)

    #print(levelArrays)
    #print('len level array', len(levelArrays))
    #print(dictChildParent)
    #print(len(arrBlockSize))
    print(dictCombParent)




def drawZoomTree(filename,rootName,numLevel):
    d = graphviz.Digraph(filename=filename,format='JPEG')
    levelOneArray = levelArrays[0]
    with d.subgraph() as s:
        s.attr('node', shape='rectangle', fillcolor='white', fontcolor='black', fixedsize='false', width='0.9')
        s.node('Whole area', label='Whole Trace')
        s.attr('node', rank='same', shape='circle', fillcolor='blanchedalmond', fontcolor='black', style='filled', fixedsize='true', width='1.2')
        s.node('Root', label=rootName)
    with d.subgraph() as s:
        s.attr('node',  shape='rectangle', fillcolor='white', fontcolor='black')
        s.node(arrBlockSize[0], label=arrBlockSize[0]+' Bytes')
        s.edge('Whole area', arrBlockSize[0], style='invis')
        s.attr('node', rank='same', shape='circle', fillcolor='cornsilk', fontcolor='black', style='filled',fixedsize='true', width='1.2')
        for nodeName,nodeLabel in levelOneArray:
            #print(nodeName,nodeLabel)
            s.node(nodeName,label=nodeLabel)
    # Level 2 and above
    arrColor =['darkseagreen1', 'bisque','darkslategray1', 'floralwhite', 'azure','lightgoldenrod1','thistle1','mintcream','mistyrose']
    for i in range(1, len(levelArrays)):
        _fillcolor=arrColor[i%len(arrColor)]
        levelTwoArray = levelArrays[i]
        #print(len(levelTwoArray))
        #print(i, levelTwoArray)
        if i < (numLevel-1):
            if(str(i)+'_0' in dictAggChild):
                with d.subgraph() as s:
                    s.attr('node',  shape='rectangle', fillcolor=_fillcolor, style='filled', fontcolor='black')
                    s.node(arrBlockSize[i], label=arrBlockSize[i])
                    for nodeName in dictAggChild[(str(i)+'_0')]:
                        s.attr('node', rank='same', shape='circle', fillcolor=_fillcolor, style='filled', fontcolor='black',fixedsize='true', width='1.5')
                        s.node(nodeName, label=nodeName)

        #if (len(levelTwoArray)<=20):
            with d.subgraph() as s:
                s.attr('node',  shape='rectangle', fillcolor=_fillcolor, style='filled', fontcolor='black')
                s.node(arrBlockSize[i], label=arrBlockSize[i]+' Bytes')
                s.edge(arrBlockSize[i-1], arrBlockSize[i], style='invis')

                for nodeName,nodeLabel in levelTwoArray:
                    if 'comb' in nodeName:
                        s.attr('node', rank='same', shape='circle', fillcolor=_fillcolor, style='filled', fontcolor='black',fixedsize='true', width='1.5')
                    else:
                        s.attr('node', rank='same', shape='circle', fillcolor=_fillcolor, style='filled', fontcolor='black',fixedsize='true', width='1.4')
                    s.node(nodeName,label=nodeLabel)
        if i==(numLevel-1):
            prevNodeGroup=''
        #if (len(levelTwoArray)>20):
            nodeGroupLead =''
            #print ( 'in i', i)
            levelTwoArray = levelArrays[i]
            listNodeGroup=[]
            arrayNodeGroup=[]
            if len(levelTwoArray) > 1:
                #tempname = [item[0] for item in levelTwoArray]
                #temp1,temp2=[tempname[i] for i in [0,1]]
                temp1, temp2 = levelTwoArray[0]
                temp2=temp1.split('_')
                prevNodeGroup=temp2[0]
                #print(prevNodeGroup)
            with d.subgraph() as b_rect:
                b_rect.attr('node',  shape='rectangle', style='filled', fillcolor='cyan', fontcolor='black')
                b_rect.node(arrBlockSize[i], label=arrBlockSize[i]+' Bytes')
                b_rect.edge(arrBlockSize[i-1], arrBlockSize[i], style='invis')
            curNodeGroup=''
            for nodeName,nodeLabel in levelTwoArray:
                #print(nodeName)
                nodeGroup=nodeName.split('_')
                curNodeGroup=nodeGroup[0]
                #print(curNodeGroup)
                if (prevNodeGroup != curNodeGroup):
                    #print('in assign new group',prevNodeGroup, curNodeGroup)
                    prevNodeGroup = curNodeGroup
                    with d.subgraph(name=curNodeGroup) as t:
                        #t.attr('graph',rankdir='TB',newrank='true',group=curNodeGroup,style='invis', shape='rectangle')
                        t.attr('node', rank='same', rankdir='TB', shape='circle', fillcolor='cyan', style='filled', fontcolor='black',fixedsize='true', width='1.5')
                        if(len(arrayNodeGroup)) >1:
                            nodeGroupLead = arrayNodeGroup[0][0]
                        for groupNodeName, groupNodeLabel in arrayNodeGroup:
                            t.node(groupNodeName,label=groupNodeLabel, rankdir='TB',group=curNodeGroup)
                            if(nodeGroupLead!=''):
                                t.edge(nodeGroupLead,groupNodeName, style='invis')
                            nodeGroupLead = groupNodeName
                    listNodeGroup=[nodeName,nodeLabel]
                    arrayNodeGroup.clear()
                    arrayNodeGroup.append(listNodeGroup)
                else:
                    listNodeGroup=[nodeName,nodeLabel]
                    arrayNodeGroup.append(listNodeGroup)
            #print('after for ', curNodeGroup, arrayNodeGroup)
            with d.subgraph(name=curNodeGroup) as s:
                s.attr('node', rank='same',  rankdir='TB', shape='circle', fillcolor='cyan', style='filled', fontcolor='black',fixedsize='true', width='1.5')
                if(len(arrayNodeGroup)) >1:
                        nodeGroupLead = arrayNodeGroup[0][0]
                        #print(nodeGroupLead)
                for groupNodeName, groupNodeLabel in arrayNodeGroup:
                    #print(groupNodeName,nodeGroupLead)
                    s.node(groupNodeName,label=groupNodeLabel, rankdir='TB',group=curNodeGroup)
                    if(nodeGroupLead.strip('_') == groupNodeName.strip('_')):
                        s.edge(nodeGroupLead,groupNodeName, style='invis')
                    nodeGroupLead = groupNodeName

    for key in dictChildParent:
        if('comb'in key):
            d.edge(dictChildParent[key], key, constraint='true',color='blue' ,style='bold',penwidth='3')
        else:
            d.edge(dictChildParent[key], key, constraint='true', style='bold')
    for key in dictCombParent:
        d.edge(dictCombParent[key],key, color='blue:invis:blue:invis:blue', style='bold', penwidth='3',arrowhead='none',constraint='false')
    d.attr(label='Minivite variant '+rootName)
    d.attr(fontsize='18')
    d.view()


arrayFileName = ['O3_v1_zoomIn_512_RUD.out', 'O3_v2_zoomIn_512_RUD.out','O3_v3_zoomIn_512_RUD.out']
#arrayFileName = ['zoomIn_V1.out', 'zoomIn_viz_V1.out']
#arrayFileName = ['zoomIn_V1.out']
with open(arrayFileName[0]) as f:
    for line in f:
        pass
    last_line = line
    #print (last_line)

data = last_line.strip().split(' ')
numLevel = int(data[2])
#print (data[2])

for fileName in arrayFileName:
    levelOneArray=[]
    levelTwoArray=[]
    levelThreeArray=[]
    dictChildParent = {}
    dictCombParent ={}
    arrBlockSize=[]
    levelArrays=[[] for _ in range(numLevel)]
    nameArray=fileName.split('_')
    variantMV=nameArray[1]
    readFile(fileName, variantMV)
    drawZoomTree(variantMV+'_zoomTree',variantMV.upper(),numLevel)
